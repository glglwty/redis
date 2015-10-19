// apps
# include "repis.app.example.h"
# include "repis.server.impl.h"

void module_init()
{
    // register replication application provider
    dsn::replication::register_replica_provider<::dsn::apps::repis_service_impl>("repis");

    // register all possible services
    dsn::register_app<::dsn::service::meta_service_app>("meta");
    dsn::register_app<::dsn::replication::replication_service_app>("replica");
    dsn::register_app<::dsn::apps::repis_client_app>("client");
    dsn::register_app<::dsn::apps::repis_perf_test_client_app>("client.perf.repis");
}

# ifndef DSN_RUN_USE_SVCHOST

int main(int argc, char** argv)
{
    module_init();
    
    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}

# else

# include <dsn/internal/module_int.cpp.h>

# endif

