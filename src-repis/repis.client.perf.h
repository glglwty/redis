# pragma once

# include "repis.client.h"
#include "protocol.h"

namespace dsn { namespace apps {  

 
class repis_perf_test_client 
    : public repis_client, 
      public ::dsn::service::perf_client_helper 
{

	using tokens = std::list<std::string>;
	using token_gen_t = std::function<tokens(int)>;
	struct command_t
	{
		const char* /* 'static */ name;
		bool write;
		token_gen_t f;
	};
	std::vector<command_t> commands;
public:
    repis_perf_test_client(
        const std::vector<::dsn::rpc_address>& meta_servers,
        const char* app_name)
        : repis_client(meta_servers, app_name)
    {
    }

    void start_test()
    {
		int rand_length = 12;
		int max_rand = 0;

		auto rand_num_factory = [rand_length, max_rand](const std::string prefix) -> token_gen_t
		{
			return[prefix, rand_length, max_rand](int) -> tokens
			{
				int rand = (max_rand == 0 ? utils::get_random64() : utils::get_random64() % max_rand) % std::numeric_limits<int>::max();
				auto ret = prefix + std::string(rand_length, '0');
				for (int i = 0; i < rand_length && rand != 0; i++)
				{
					*(ret.rbegin() + i) = rand % 10;
					rand /= 10;
				}
				return tokens{ret};
			};
		};

		auto repeat_factory = [](int num, std::function<tokens(int)> f) -> token_gen_t
		{
			return [num, f](int payload_bytes) -> tokens
			{
				tokens result;
				for (int i = 0; i < num; i++)
				{
					result.splice(result.end(), f(payload_bytes));
				}
				return result;
			};
		};
		auto literal_factory = [](const std::string literal) -> token_gen_t
		{
			return[literal](int) -> tokens
			{
				return tokens{literal};
			};
		};
		auto payload = [](int payload_bytes) -> tokens
		{
			return tokens{std::string(payload_bytes, 'x')};
		};
		auto combine_factory = [](std::initializer_list<token_gen_t> flist) -> token_gen_t
		{
			std::list<token_gen_t> fs(flist);
			return[fs](int payload_bytes) -> tokens {
				tokens result;
				for (auto &f : fs)
				{
					result.splice(result.end(), f(payload_bytes));
				};
				return result;
			};
		};
		
		commands = {
			{ "ping", false, literal_factory("ping") },
			{ "set", true, combine_factory({ literal_factory("set"), rand_num_factory("key:"), payload }) },
			{ "get", false, combine_factory({ literal_factory("get"), rand_num_factory("key:") }) },
			{ "incr", true, combine_factory({ literal_factory("incr"), rand_num_factory("counter:") }) },
			{ "lpush", true, combine_factory({ literal_factory("lpush"), literal_factory("mylist"), payload }) },
			{ "lpop", true, combine_factory({ literal_factory("lpop"), literal_factory("mylist") }) },
			{ "sadd", true, combine_factory({ literal_factory("sadd"), literal_factory("myset"), rand_num_factory("element:") }) },
			{ "spop", true, combine_factory({ literal_factory("spop"), literal_factory("myset") }) },
			{ "lpush for lrange", true, combine_factory({ literal_factory("lpush"), literal_factory("mylist"), payload }) },
			{ "lrange_100", false, combine_factory({ literal_factory("lrange"), literal_factory("mylist"), literal_factory("0"), literal_factory("99") }) },
			{ "lrange_300", false, combine_factory({ literal_factory("lrange"), literal_factory("mylist"), literal_factory("0"), literal_factory("299") }) },
			{ "lrange_500", false, combine_factory({ literal_factory("lrange"), literal_factory("mylist"), literal_factory("0"), literal_factory("449") }) },
			{ "lrange_600", false, combine_factory({ literal_factory("lrange"), literal_factory("mylist"), literal_factory("0"), literal_factory("599") }) },
			{ "mset", true, combine_factory({ literal_factory("mset"), repeat_factory(10, combine_factory({ rand_num_factory("key:"), payload })) }) },
		};

        perf_test_suite s;
        std::vector<perf_test_suite> suits;

		for (auto& command : commands)
		{
			s.name = command.name;
			s.config_section = "task.RPC_REPIS_TEST";
			s.send_one = [command, this](int payload_bytes)
			{
				void *ctx = this->prepare_send_one();
				std::string cmd = build_command(command.f(payload_bytes));
				::dsn::blob req(cmd.c_str(), 0, cmd.length());
				if (command.write)
				{
					begin_write(req, ctx, _timeout_ms);
				} else
				{
					begin_read(req, ctx, _timeout_ms);
				}
			};
			s.cases.clear();
			load_suite_config(s);
			suits.push_back(s);
		}
		start(suits);
    }                

    virtual void end_read(
        ::dsn::error_code err,
        const ::dsn::blob& resp,
        void* context) override
    {
        end_send_one(context, err);
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