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
			: repis_service(replica), libredis_instance(nullptr), _max_checkpoint_count(3), _is_open(false)
		{
		}

		void repis_service_impl::on_read(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
			service::zauto_lock _(_lock);
			libredis_set_instance(libredis_instance);
			auto result = libredis_call(request.data(), request.length());
			//std::cout << "on_read! request = " << request.data() << " length = " << result.len << "content = " << result.buf << std::endl;
			reply(::dsn::blob(result.buf, 0, result.len));
			libredis_drop_reply(&result);
		}

		void repis_service_impl::on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			{
				dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
				reply_t result;
				{
					service::zauto_lock _(_lock);
					libredis_set_instance(libredis_instance);
					result = libredis_call(request.data(), request.length());
					++_last_committed_decree;
					//std::cout << "on_write! request = " << request.data() << " length = " << result.len << "content = " << result.buf << std::endl;
				}
				flush(true);
				reply(::dsn::blob(result.buf, 0, result.len));
				libredis_drop_reply(&result);
			}
		}

		int  repis_service_impl::open(bool create_new)
		{
			std::string rdb_dir = utils::filesystem::path_combine(data_dir(), "rdb");
			dassert(!_is_open, "repis service %s is already opened", data_dir().c_str());
			if (create_new) {
				std::cout << "trying to create a new instance!!!!!!!!!!!!" << std::endl;
				auto& dir = data_dir();
				dsn::utils::filesystem::remove_path(dir);
				dsn::utils::filesystem::create_directory(dir);

				char *argv[] = { "redis.exe"};
				_last_committed_decree = 0;
				_checkpoints.clear();
				
				libredis_instance = libredis_new_instance(1, argv);
				std::cout << "libredis instance created!!!!!" << std::endl;
			}
			else
			{
				std::cout << "trying to load old database" << std::endl;
				dassert(utils::filesystem::directory_exists(rdb_dir), "rdb dir does not exist");
				
				char *argv[] = { "redis.exe", "--dir", const_cast<char*>(rdb_dir.c_str())};
				libredis_instance = libredis_new_instance(sizeof(argv) / sizeof(char*), argv);
				std::cout << "libredis instance created!!!!!" << std::endl;
				{
					std::string serial_number_path = utils::filesystem::path_combine(rdb_dir, "serial_number");
					std::ifstream serial_number_stream(serial_number_path);
					replication::decree temporary_decree;
					if (serial_number_stream >> temporary_decree)
					{
						_last_committed_decree = temporary_decree;
					}
					else
					{
						std::cout << "load database failed" << std::endl;
						return 1;
					}
				}
			}
			_last_durable_decree = parse_for_checkpoints();
			_is_open = true;
			return 0;
		}

		int  repis_service_impl::close(bool clear_state)
		{
			_is_open = false;
			if (clear_state) {
				_checkpoints.clear();
				if (utils::filesystem::directory_exists(data_dir())) {
					utils::filesystem::remove_path(data_dir());
				}
			}
			//redis does not support close! yeah! you have to shutdown the process!
			return 0;
		}

		// flush is always done in the same single thread, so
		int  repis_service_impl::flush(bool wait)
		{
			std::cout << "flushing..replica...." << replica_name() << std::endl;
			service::zauto_lock _(_lock);
			libredis_set_instance(libredis_instance);
			dassert(_is_open, "repis service %s is not ready", data_dir().c_str());
			if (last_durable_decree() == last_committed_decree()) {
				return 0;
			}
			//XXX rrdb doesn't use a temporary folder & renaming. Review this one later.
			auto chkpt_dir = utils::filesystem::path_combine(data_dir(), chkpt_get_dir_name(last_committed_decree()));
			auto rdb_dir = utils::filesystem::path_combine(data_dir(), "rdb");


			if (true_flush(chkpt_dir) != 0 || true_flush(rdb_dir) != 0)
			{
				if (utils::filesystem::directory_exists(chkpt_dir))
					utils::filesystem::remove_path(chkpt_dir);
				if (utils::filesystem::directory_exists(rdb_dir))
					utils::filesystem::remove_path(rdb_dir);

			}
			_last_durable_decree = last_committed_decree();
			_checkpoints.push_back(last_committed_decree());

			gc_checkpoints();
			std::cout << "flush succeed!\n" << std::endl;
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
			dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
			service::zauto_lock _(_lock);
			if (_checkpoints.size() > 0)
			{
				replication::decree ch = *_checkpoints.rbegin();
				auto dir = chkpt_get_dir_name(ch);

				

				auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);

				dassert(utils::filesystem::path_exists(chkpt_dir), "checkponit dir does not exist");
				auto err = utils::filesystem::get_subfiles(chkpt_dir, state.files, true);
				dassert(err, "list files in chkpoint dir %s failed", chkpt_dir.c_str());
			} else
			{
				dassert(false, "");
			}

			return 0;
		}

		int  repis_service_impl::apply_learn_state(::dsn::replication::learn_state& state)
		{
			service::zauto_lock _(_lock);
			std::cout << "trying to recovery...... from " << data_dir() << std::endl;
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
			std::string new_dir = ::dsn::utils::filesystem::path_combine(data_dir(), "rdb");

			if (!::dsn::utils::filesystem::rename_path(learn_dir, new_dir))
			{
				derror("rename %s to %s failed", learn_dir.c_str(), new_dir.c_str());
				return ERR_FILE_OPERATION_FAILED;
			}
			
			std::cout << "rename succeed ~!!! check it out!" << std::endl;

			// reopen the db with the new checkpoint files
			if (state.files.size() > 0) {
				err = open(false);
				std::cout << "learned decree" << last_committed_decree() << std::endl;
			}
			else
				err = open(true);

			if (err != 0)
			{
				derror("open db %s failed, err = %d", data_dir().c_str(), err);
				return err;
			}
			flush(true);

			return 0;
		}

		int repis_service_impl::true_flush(const std::string& chkpt_dir)
		{
			if (utils::filesystem::directory_exists(chkpt_dir))
				utils::filesystem::remove_path(chkpt_dir);

			utils::filesystem::create_directory(chkpt_dir);
			libredis_call_wrapper(build_command({ "config", "set", "dir", chkpt_dir }));

			if (!libredis_boolean_call_wrapper(build_command({ "save" })))
			{
				std::cout << "oh my god redis flush failed" << std::endl;
				getchar();
				if (utils::filesystem::directory_exists(chkpt_dir))
					utils::filesystem::remove_path(chkpt_dir);
				return 1;
			}

			{
				auto serial_num_filename = utils::filesystem::path_combine(chkpt_dir, "serial_number");
				//XXX use builtin aio here?
				std::ofstream serial_file(serial_num_filename.c_str());
				if (!serial_file.is_open())
				{
					std::cout << "cannot open serial file" << std::endl;

					getchar();
					if (utils::filesystem::directory_exists(chkpt_dir))
						utils::filesystem::remove_path(chkpt_dir);
					return 1;
				}
				if (!(serial_file << last_committed_decree()))
				{
					std::cout << "cannot output lastcommitted decree" << std::endl;

					getchar();

					if (utils::filesystem::directory_exists(chkpt_dir))
						utils::filesystem::remove_path(chkpt_dir);
					return 1;
				}
			}
			return 0;
		}

		replication::decree repis_service_impl::parse_for_checkpoints()
		{
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
