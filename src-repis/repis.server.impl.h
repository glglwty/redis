# pragma once

# include "repis.server.h"
# include <vector>

namespace dsn {
	namespace apps {
		class repis_service_impl : public repis_service
		{
		public:
			repis_service_impl(::dsn::replication::replica* replica);


			virtual void on_read(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply) override;
			virtual void on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply) override;

			virtual int  open(bool create_new) override;
			virtual int  close(bool clear_state) override;
			virtual int  flush(bool wait) override;
			virtual void on_empty_write() override;
			virtual void prepare_learning_request(/*out*/ blob& learn_req) override;
			virtual int  get_learn_state(::dsn::replication::decree start,
				const blob& learn_req, /*out*/ ::dsn::replication::learn_state& state) override;
			virtual int  apply_learn_state(::dsn::replication::learn_state& state) override;

		private:

			void *libredis_instance;



		};


		// --------- inline implementations -----------------
	}
}
