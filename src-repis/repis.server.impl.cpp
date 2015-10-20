# include "repis.server.impl.h"
# include <algorithm>
# include <dsn/cpp/utils.h>

extern "C" {
	#include "../src/libredis.h"
}

# ifdef __TITLE__
# undef __TITLE__
# endif
#define __TITLE__ "repis.server.impl"



namespace dsn {
	namespace apps {

		repis_service_impl::repis_service_impl(::dsn::replication::replica* replica)
			: repis_service(replica)	
		{
		}

		void repis_service_impl::on_read(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			libredis_set_instance(libredis_instance);
			auto result = libredis_call(request.data(), request.length());
			//std::cout << "on_read! request = " << request.data() << " length = " << result.len << "content = " << result.buf << std::endl;
			reply(::dsn::blob(result.buf, 0, result.len));
			//XXX: can we drop the result immediately?
			libredis_drop_reply(&result);
		}

		void repis_service_impl::on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			libredis_set_instance(libredis_instance);
			auto result = libredis_call(request.data(), request.length());
			++_last_committed_decree;

			//std::cout << "on_write! request = " << request.data() << " length = " << result.len << "content = " << result.buf << std::endl;
			reply(::dsn::blob(result.buf, 0, result.len));
			libredis_drop_reply(&result);
		}

		int  repis_service_impl::open(bool create_new)
		{
			_last_committed_decree = 0;
			_last_durable_decree = 0;
			char *argv[] = { "redis.exe" };
			std::cout << "trying to create a new instance" << std::endl;
			libredis_instance = libredis_new_instance(1, argv);
			std::cout << "libredis instance created!!!!!" << std::endl;
			return 0;
		}

		int  repis_service_impl::close(bool clear_state)
		{
			return 0;
		}

		// flush is always done in the same single thread, so
		int  repis_service_impl::flush(bool wait)
		{
			_last_durable_decree = replication::decree(_last_committed_decree);
			return 0;
		}

		void repis_service_impl::on_empty_write()
		{
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

			return 0;
		}

		int  repis_service_impl::apply_learn_state(::dsn::replication::learn_state& state)
		{
			return 0;
		}

	}
}
