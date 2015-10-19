# pragma once
# include <dsn/dist/replication.h>
# include "repis.code.definition.h"
# include <iostream>

namespace dsn { namespace apps { 
class repis_service 
    : public ::dsn::replication::replication_app_base
{
public:
    repis_service(::dsn::replication::replica* replica) 
        : ::dsn::replication::replication_app_base(replica)
    {
        open_service();
    }
    
    virtual ~repis_service() 
    {
        close_service();
    }

protected:
    // all service handlers to be implemented further
    // RPC_REPIS_REPIS_READ 
    virtual void on_read(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
    {
        std::cout << "... exec RPC_REPIS_REPIS_READ ... (not implemented) " << std::endl;
        ::dsn::blob resp;
        reply(resp);
    }
    // RPC_REPIS_REPIS_WRITE 
    virtual void on_write(const ::dsn::blob& request, ::dsn::rpc_replier<::dsn::blob>& reply)
    {
        std::cout << "... exec RPC_REPIS_REPIS_WRITE ... (not implemented) " << std::endl;
        ::dsn::blob resp;
        reply(resp);
    }
    
public:
    void open_service()
    {
        this->register_async_rpc_handler(RPC_REPIS_REPIS_READ, "read", &repis_service::on_read);
        this->register_async_rpc_handler(RPC_REPIS_REPIS_WRITE, "write", &repis_service::on_write);
    }

    void close_service()
    {
        this->unregister_rpc_handler(RPC_REPIS_REPIS_READ);
        this->unregister_rpc_handler(RPC_REPIS_REPIS_WRITE);
    }
};

} } 