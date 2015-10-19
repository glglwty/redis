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
			reply(::dsn::blob("fuck off", 0, 8));
		}

		void repis_service_impl::on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
		{
			libredis_set_instance(libredis_instance);
			++_last_committed_decree;
			reply(::dsn::blob("fuck off", 0, 8));
		}

		int  repis_service_impl::open(bool create_new)
		{
			_last_committed_decree = 0;
			_last_durable_decree = 0;
			char *argv[] = { "redis.exe" };
			std::cout << "trying to create a new instance" << std::endl;
			libredis_instance = libredis_new_instance(1, argv);
			std::cout << "libredis instance created!!!!!" << std::endl;
			getchar();
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
