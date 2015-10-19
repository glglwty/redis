# pragma once
# include <dsn/dist/replication.h>
# include "repis.code.definition.h"
# include <iostream>

namespace dsn { namespace apps { 
class repis_client 
    : public ::dsn::replication::replication_app_client_base
{
public:
    repis_client(
        const std::vector<::dsn::rpc_address>& meta_servers,
        const char* replicated_app_name)
        : ::dsn::replication::replication_app_client_base(meta_servers, replicated_app_name) 
    {
    }
	
    virtual ~repis_client() {}
    
    // from requests to partition index
    // PLEASE DO RE-DEFINE THEM IN A SUB CLASS!!!
    virtual int get_partition_index(const ::dsn::blob& key) { return 0;};

    // ---------- call RPC_REPIS_REPIS_READ ------------
    // - synchronous 
    ::dsn::error_code read(
        const ::dsn::blob& request, 
        /*out*/ ::dsn::blob& resp, 
        int timeout_milliseconds = 0, 
        ::dsn::replication::read_semantic_t read_semantic = ::dsn::replication::read_semantic_t::ReadLastUpdate 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::read<::dsn::blob, ::dsn::blob>(
            get_partition_index(request),
            RPC_REPIS_REPIS_READ,
            request,
            nullptr,
            nullptr,
            nullptr,
            timeout_milliseconds,
            0, 
            read_semantic 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack ::dsn::blob and ::dsn::blob 
    ::dsn::task_ptr begin_read(
        const ::dsn::blob& request,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0,  
        ::dsn::replication::read_semantic_t read_semantic = ::dsn::replication::read_semantic_t::ReadLastUpdate 
        )
    {
        return ::dsn::replication::replication_app_client_base::read<repis_client, ::dsn::blob, ::dsn::blob>(
            get_partition_index(request),
            RPC_REPIS_REPIS_READ, 
            request,
            this,
            &repis_client::end_read, 
            context,
            timeout_milliseconds,
            reply_hash, 
            read_semantic 
            );
    }

    virtual void end_read(
        ::dsn::error_code err, 
        const ::dsn::blob& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_REPIS_REPIS_READ err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_REPIS_REPIS_READ ok" << std::endl;
        }
    }
    
    // - asynchronous with on-heap std::shared_ptr<::dsn::blob> and std::shared_ptr<::dsn::blob> 
    ::dsn::task_ptr begin_read2(
        std::shared_ptr<::dsn::blob>& request,         
        int timeout_milliseconds = 0, 
        int reply_hash = 0, 
        ::dsn::replication::read_semantic_t read_semantic = ::dsn::replication::read_semantic_t::ReadLastUpdate 
        )
    {
        return ::dsn::replication::replication_app_client_base::read<repis_client, ::dsn::blob, ::dsn::blob>(
            get_partition_index(*request),
            RPC_REPIS_REPIS_READ,
            request,
            this,
            &repis_client::end_read2, 
            timeout_milliseconds,
            reply_hash, 
            read_semantic 
            );
    }

    virtual void end_read2(
        ::dsn::error_code err, 
        std::shared_ptr<::dsn::blob>& request, 
        std::shared_ptr<::dsn::blob>& resp)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_REPIS_REPIS_READ err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_REPIS_REPIS_READ ok" << std::endl;
        }
    }
    

    // ---------- call RPC_REPIS_REPIS_WRITE ------------
    // - synchronous 
    ::dsn::error_code write(
        const ::dsn::blob& request, 
        /*out*/ ::dsn::blob& resp, 
        int timeout_milliseconds = 0 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::write<::dsn::blob, ::dsn::blob>(
            get_partition_index(request),
            RPC_REPIS_REPIS_WRITE,
            request,
            nullptr,
            nullptr,
            nullptr,
            timeout_milliseconds,
            0 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack ::dsn::blob and ::dsn::blob 
    ::dsn::task_ptr begin_write(
        const ::dsn::blob& request,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0 
        )
    {
        return ::dsn::replication::replication_app_client_base::write<repis_client, ::dsn::blob, ::dsn::blob>(
            get_partition_index(request),
            RPC_REPIS_REPIS_WRITE, 
            request,
            this,
            &repis_client::end_write, 
            context,
            timeout_milliseconds,
            reply_hash 
            );
    }

    virtual void end_write(
        ::dsn::error_code err, 
        const ::dsn::blob& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_REPIS_REPIS_WRITE err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_REPIS_REPIS_WRITE ok" << std::endl;
        }
    }
    
    // - asynchronous with on-heap std::shared_ptr<::dsn::blob> and std::shared_ptr<::dsn::blob> 
    ::dsn::task_ptr begin_write2(
        std::shared_ptr<::dsn::blob>& request,         
        int timeout_milliseconds = 0, 
        int reply_hash = 0 
        )
    {
        return ::dsn::replication::replication_app_client_base::write<repis_client, ::dsn::blob, ::dsn::blob>(
            get_partition_index(*request),
            RPC_REPIS_REPIS_WRITE,
            request,
            this,
            &repis_client::end_write2, 
            timeout_milliseconds,
            reply_hash 
            );
    }

    virtual void end_write2(
        ::dsn::error_code err, 
        std::shared_ptr<::dsn::blob>& request, 
        std::shared_ptr<::dsn::blob>& resp)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_REPIS_REPIS_WRITE err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_REPIS_REPIS_WRITE ok" << std::endl;
        }
    }
    
};

} } 