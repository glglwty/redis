# pragma once
# include "repis.client.h"
# include "repis.client.perf.h"
# include "repis.server.h"

namespace dsn { namespace apps { 
// client app example
class repis_client_app : 
    public ::dsn::service_app,
    public virtual ::dsn::clientlet
{
public:
    repis_client_app()
    {
        _repis_client = nullptr;
    }
    
    ~repis_client_app() 
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv)
    {
        if (argc < 2)
            return ::dsn::ERR_INVALID_PARAMETERS;

        std::vector<::dsn::rpc_address> meta_servers;
        ::dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);
        
        _repis_client = new repis_client(meta_servers, argv[1]);
        _timer = ::dsn::tasking::enqueue(LPC_REPIS_TEST_TIMER, this, &repis_client_app::on_test_timer, 0, 0, 1000);
        return ::dsn::ERR_OK;
    }

    virtual void stop(bool cleanup = false)
    {
        _timer->cancel(true);
 
        if (_repis_client != nullptr)
        {
            delete _repis_client;
            _repis_client = nullptr;
        }
    }

    void on_test_timer()
    {
        // test for service 'repis'
        {
            ::dsn::blob req;
            //sync:
            ::dsn::blob resp;
            auto err = _repis_client->read(req, resp);
            std::cout << "call RPC_REPIS_REPIS_READ end, return " << err.to_string() << std::endl;
            //async: 
            //_repis_client->begin_read(req);
           
        }
        {
            ::dsn::blob req;
            //sync:
            ::dsn::blob resp;
            auto err = _repis_client->write(req, resp);
            std::cout << "call RPC_REPIS_REPIS_WRITE end, return " << err.to_string() << std::endl;
            //async: 
            //_repis_client->begin_write(req);
           
        }
    }

private:
    ::dsn::task_ptr _timer;
    ::dsn::rpc_address _server;
    
    repis_client *_repis_client;
};

class repis_perf_test_client_app : 
    public ::dsn::service_app,
    public virtual ::dsn::clientlet
{
public:
    repis_perf_test_client_app()
    {
        _repis_client = nullptr;
    }

    ~repis_perf_test_client_app()
    {
        stop();
    }

    virtual ::dsn::error_code start(int argc, char** argv)
    {
        if (argc < 2)
            return ::dsn::ERR_INVALID_PARAMETERS;

        std::vector<::dsn::rpc_address> meta_servers;
        ::dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);

        _repis_client = new repis_perf_test_client(meta_servers, argv[1]);
        _repis_client->start_test();
        return ::dsn::ERR_OK;
    }

    virtual void stop(bool cleanup = false)
    {
        if (_repis_client != nullptr)
        {
            delete _repis_client;
            _repis_client = nullptr;
        }
    }
    
private:
    repis_perf_test_client *_repis_client;
};

} } 