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
			virtual void prepare_learning_request(/*out*/ blob& learn_req) override;
			virtual int  get_learn_state(::dsn::replication::decree start,
				const blob& learn_req, /*out*/ ::dsn::replication::learn_state& state) override;
			virtual int  apply_learn_state(::dsn::replication::learn_state& state) override;

		private:
			int true_flush(const std::string &dir);
			replication::decree parse_for_checkpoints();
			void gc_checkpoints();

			service::zlock _lock;
			void *libredis_instance;
			//rrdb does not use lock to protect this vector. I don't know how checkpoint functions are called so just following rrdb should be OK.
			std::vector<replication::decree> _checkpoints;
			const int             _max_checkpoint_count;
			std::atomic<bool> _is_open;
		};


		// --------- inline implementations -----------------
	}
}
