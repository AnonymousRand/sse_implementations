#pragma once

#include <concepts>

#include "schemes/n_log_n/n_log_n_base_server.h"


namespace log_src_i_star {


template <IsDbTuple DbTuple>
class UnderlyServer : public NLogNBaseServer<DbTuple> {
public:
    using NLogNBaseServer<DbTuple>::NLogNBaseServer;
};


} // namespace `log_src_i_star`
