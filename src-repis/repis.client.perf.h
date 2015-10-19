# pragma once

# include "repis.client.h"

namespace dsn { namespace apps {  

 
class repis_perf_test_client 
    : public repis_client, 
      public ::dsn::service::perf_client_helper 
{
public:
    repis_perf_test_client(
        const std::vector<::dsn::rpc_address>& meta_servers,
        const char* app_name)
        : repis_client(meta_servers, app_name)
    {
    }

    void start_test()
    {
        perf_test_suite s;
        std::vector<perf_test_suite> suits;

        s.name = "repis.read";
        s.config_section = "task.RPC_REPIS_REPIS_READ";
        s.send_one = [this](int payload_bytes){this->send_one_read(payload_bytes); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        s.name = "repis.write";
        s.config_section = "task.RPC_REPIS_REPIS_WRITE";
        s.send_one = [this](int payload_bytes){this->send_one_write(payload_bytes); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        start(suits);
    }                

    void send_one_read(int payload_bytes)
    {
        void* ctx = prepare_send_one();
        ::dsn::blob req;
        // TODO: randomize the value of req
        // auto rs = random64(0, 10000000);
        // std::stringstream ss;
        // ss << "key." << rs;
        // req = ss.str();
        
        begin_read(req, ctx, _timeout_ms);
    }

    virtual void end_read(
        ::dsn::error_code err,
        const ::dsn::blob& resp,
        void* context) override
    {
        end_send_one(context, err);
    }

    void send_one_write(int payload_bytes)
    {
        void* ctx = prepare_send_one();
        ::dsn::blob req;
        // TODO: randomize the value of req
        // auto rs = random64(0, 10000000);
        // std::stringstream ss;
        // ss << "key." << rs;
        // req = ss.str();
        
        begin_write(req, ctx, _timeout_ms);
    }

    virtual void end_write(
        ::dsn::error_code err,
        const ::dsn::blob& resp,
        void* context) override
    {
        end_send_one(context, err);
    }
};

} } 