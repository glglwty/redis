# pragma once
# include <dsn/service_api_cpp.h>
# include "repis.types.h"

namespace dsn { namespace apps { 
    // define your own thread pool using DEFINE_THREAD_POOL_CODE(xxx)
    // define RPC task code for service 'repis'
    DEFINE_TASK_CODE_RPC(RPC_REPIS_REPIS_READ, TASK_PRIORITY_COMMON, ::dsn::THREAD_POOL_DEFAULT)
    DEFINE_TASK_CODE_RPC(RPC_REPIS_REPIS_WRITE, TASK_PRIORITY_COMMON, ::dsn::THREAD_POOL_DEFAULT)
    // test timer task code
    DEFINE_TASK_CODE(LPC_REPIS_TEST_TIMER, TASK_PRIORITY_COMMON, ::dsn::THREAD_POOL_DEFAULT)
} } 
