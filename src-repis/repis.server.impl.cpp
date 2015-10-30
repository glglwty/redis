# include "repis.server.impl.h"
# include <algorithm>
#include <fstream>
# include <dsn/cpp/utils.h>
# include "protocol.h"

extern "C" {
	#include "../src/libredis.h"
}

# ifdef __TITLE__
# undef __TITLE__
# endif
#define __TITLE__ "repis.server.impl"



namespace dsn {
	namespace apps {

		static bool reply_prefix_same(reply_t &reply, const std::string& expected)
		{
			return reply.len >= expected.length() && strncmp(reply.buf, expected.c_str(), expected.length()) == 0;
		}

		static bool chkpt_init_from_dir(const char* name, replication::decree& last_seq)
		{
			return 1 == sscanf(name, "checkpoint.%lld", &last_seq);
		}

		static void libredis_call_wrapper(const std::string& command)
		{
			auto reply = libredis_call(command.c_str(), command.size());
			libredis_drop_reply(&reply);
		}

		static bool libredis_boolean_call_wrapper(const std::string& command)
		{
			auto reply = libredis_call(command.c_str(), command.size());
			bool ok = reply_prefix_same(reply, "+OK");
			libredis_drop_reply(&reply);
			return ok;
		}

		static std::string chkpt_get_dir_name(int64_t seq_num)
		{
			char buffer[256];
			sprintf(buffer, "checkpoint.%lld", seq_num);
			return std::string(buffer);
		}
		
		repis_service_impl::repis_service_impl(::dsn::replication::replica* replica)
			: repis_service(replica), libredis_instance(nullptr), _max_checkpoint_count(3), _is_open(false), _lock(true)
		{
		}

		void repis_service_impl::on_read(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			service::zauto_lock _(_lock);
			dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
			libredis_set_instance(libredis_instance);
			auto result = libredis_call(request.data(), request.length());
			reply(::dsn::blob(result.buf, 0, result.len));
			libredis_drop_reply(&result);
		}

		void repis_service_impl::on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			service::zauto_lock _(_lock);
			dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
			libredis_set_instance(libredis_instance);
			auto result = libredis_call(request.data(), request.length());
			_last_committed_decree.fetch_add(1);
			reply(::dsn::blob(result.buf, 0, result.len));
			libredis_drop_reply(&result);
		}

		int  repis_service_impl::open(bool create_new)
		{
			service::zauto_lock _(_lock);
			dassert(!_is_open, "repis service %s is already opened", data_dir().c_str());
			if (create_new) {
				auto& dir = data_dir();
				dsn::utils::filesystem::remove_path(dir);
				dsn::utils::filesystem::create_directory(dir);
				char *argv[] = { "redis.exe"};
				_last_committed_decree = 0;
				_checkpoints.clear();
				if (nullptr == (libredis_instance = libredis_new_instance(nullptr, 1, argv))) {
					derror("cannot create an empty redis instance");
					return -1;
				}
				dinfo("libredis instance created");
			}
			else
			{
				std::string load_path;
				_last_durable_decree = parse_for_checkpoints();
				if (!_checkpoints.empty()) {
					load_path = utils::filesystem::path_combine(data_dir(), chkpt_get_dir_name(*_checkpoints.rbegin()));
					replication::decree temporary_decree;
					if (std::ifstream(utils::filesystem::path_combine(load_path, "serial_number")) >> temporary_decree)
					{
						_last_committed_decree = temporary_decree;
					}
					else
					{
						dwarn("load database failed");
						return 1;
					}
				}
				char *argv[] = { "redis.exe" };
				if (nullptr == (libredis_instance = libredis_new_instance(load_path.empty() ? nullptr : utils::filesystem::path_combine(load_path, "dump.rdb").c_str(), 1, argv)))
				{
					derror("cannot create an empty redis instance");
					return -1;
				}
				dinfo("libredis instance created");
			}
			_is_open = true;
			gc_checkpoints();
			dassert(last_committed_decree() == last_durable_decree(), "open postcondition test failed");
			return 0;
		}

		int  repis_service_impl::close(bool clear_state)
		{
			service::zauto_lock _(_lock);
			if (_is_open) {
				if (clear_state) {
					_checkpoints.clear();
					if (utils::filesystem::directory_exists(data_dir())) {
						utils::filesystem::remove_path(data_dir());
					}
				}
				_is_open = false;
			}
			//redis does not support close! yeah! you have to shutdown the process!
			return 0;
		}

		// flush is always done in the same single thread, so
		int  repis_service_impl::flush(bool wait)
		{
			service::zauto_lock _(_lock);
            dinfo("flushing replica");
			libredis_set_instance(libredis_instance);
			dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
			if (last_durable_decree() == last_committed_decree()) {
				return 0;
			}
			auto temp_dir1 = utils::filesystem::path_combine(data_dir(), "tmp_dir1");
			auto chkpt_dir = utils::filesystem::path_combine(data_dir(), chkpt_get_dir_name(last_committed_decree()));

			if (true_flush(temp_dir1))
			{
				if (utils::filesystem::directory_exists(temp_dir1))
					utils::filesystem::remove_path(temp_dir1);
				return 1;
			} 
            while (!utils::filesystem::rename_path(temp_dir1, chkpt_dir))
            {
                dwarn("rename checkpoint dir failed");
            }
			_last_durable_decree = last_committed_decree();
			_checkpoints.push_back(last_committed_decree());
			gc_checkpoints();
			dassert(last_committed_decree() == last_durable_decree(), "flush postcondition test failed");
			return 0;
		}

		void repis_service_impl::prepare_learning_request(/*out*/ blob& learn_req)
		{
			// nothing to do
		}

		int  repis_service_impl::get_learn_state(
			::dsn::replication::decree start,
			const blob& learn_req,
			/*out*/ ::dsn::replication::learn_state& state)
		{
			service::zauto_lock _(_lock);
			dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
			if (_checkpoints.empty())
			{
				flush(true);
			}
			auto chkpt_dir = utils::filesystem::path_combine(data_dir(), chkpt_get_dir_name(*_checkpoints.rbegin()));
			dassert(utils::filesystem::path_exists(chkpt_dir), "checkponit dir does not exist");
			auto err = utils::filesystem::get_subfiles(chkpt_dir, state.files, true);
			dassert(err, "list files in chkpoint dir %s failed", chkpt_dir.c_str());
			return 0;
		}

		int  repis_service_impl::apply_learn_state(::dsn::replication::learn_state& state)
		{
			service::zauto_lock _(_lock);
			int err = 0;
			if (_is_open)
			{
				// clear db
				err = close(true);
				if (err != 0)
				{
					derror("clear db %s failed, err = %d", data_dir().c_str(), err);
					return err;
				}
			}
			else
			{
				::dsn::utils::filesystem::remove_path(data_dir());
			}
			// create data dir first
			::dsn::utils::filesystem::create_directory(data_dir());
			// move learned files from learn_dir to data_dir
			std::string learn_dir = ::dsn::utils::filesystem::remove_file_name(*state.files.rbegin());
			replication::decree target_decree;
			if (!(std::ifstream(utils::filesystem::path_combine(learn_dir, "serial_number")) >> target_decree))
			{

				derror("invalid learn state");
				return ERR_LEARN_FILE_FALED;
			}
			std::string new_dir = ::dsn::utils::filesystem::path_combine(data_dir(), chkpt_get_dir_name(target_decree));

			if (!::dsn::utils::filesystem::rename_path(learn_dir, new_dir))
			{
				derror("rename %s to %s failed", learn_dir.c_str(), new_dir.c_str());
				return ERR_FILE_OPERATION_FAILED;
			}
			
			// reopen the db with the new checkpoint files
			if (state.files.size() > 0) {
				err = open(false);
			}
            else {
                err = open(true);
            }

			if (err != 0)
			{
				derror("open db %s failed, err = %d", data_dir().c_str(), err);
				return err;
			}
			dassert(last_committed_decree() >= last_durable_decree(), "apply learn state postcondition test failed\n");
			return 0;
		}

		int repis_service_impl::true_flush(const std::string& chkpt_dir)
		{
			service::zauto_lock _(_lock);
			if (utils::filesystem::directory_exists(chkpt_dir))
				utils::filesystem::remove_path(chkpt_dir);

			utils::filesystem::create_directory(chkpt_dir);

			if (libredis_save(utils::filesystem::path_combine(chkpt_dir, "dump.rdb").c_str()) != 0)
			{
				dwarn("redis flush failed");
		    	if (utils::filesystem::directory_exists(chkpt_dir))
				    utils::filesystem::remove_path(chkpt_dir);
				return 1;
		    }
			{
				auto serial_num_filename = utils::filesystem::path_combine(chkpt_dir, "serial_number");	;
                if (!(std::ofstream(serial_num_filename.c_str()) << last_committed_decree()))
				{
                    dwarn("write last committed decree failed");
					if (utils::filesystem::directory_exists(chkpt_dir))
						utils::filesystem::remove_path(chkpt_dir);
					return 1;
				}
			}
			return 0;
		}

		replication::decree repis_service_impl::parse_for_checkpoints()
		{
			service::zauto_lock _(_lock);
			std::vector<std::string> dirs;
			utils::filesystem::get_subdirectories(data_dir(), dirs, false);

			_checkpoints.clear();
			for (auto& d : dirs)
			{
				replication::decree temporary_decree;
				std::string d1 = d;
				d1 = d1.substr(data_dir().length() + 1);
				if (chkpt_init_from_dir(d1.c_str(), temporary_decree))
				{
					_checkpoints.push_back(temporary_decree);
				}
			}

			std::sort(_checkpoints.begin(), _checkpoints.end());

			return _checkpoints.size() > 0 ? *(_checkpoints.rbegin()) : 0;
		}

		void repis_service_impl::gc_checkpoints()
		{
			service::zauto_lock _(_lock);
			while (_checkpoints.size() > _max_checkpoint_count)
			{
				auto old_cpt = chkpt_get_dir_name(*_checkpoints.begin());
				auto old_cpt_dir = utils::filesystem::path_combine(data_dir(), old_cpt);
				if (utils::filesystem::directory_exists(old_cpt_dir))
				{
					if (utils::filesystem::remove_path(old_cpt_dir))
					{
						dinfo("%s: checkpoint %s removed", data_dir().c_str(), old_cpt_dir.c_str());
						_checkpoints.erase(_checkpoints.begin());
					}
					else
					{
						derror("%s: remove checkpoint %s failed", data_dir().c_str(), old_cpt_dir.c_str());
						break;
					}
				}
				else
				{
					derror("%s: checkpoint %s does not exist ...", data_dir().c_str(), old_cpt_dir.c_str());
					_checkpoints.erase(_checkpoints.begin());
				}
			}
		}
	}
}
