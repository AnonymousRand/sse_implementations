#pragma once

#include <concepts>
#include <memory>

#include "utils/db.h"
#include "utils/sse_utils.h"


struct Benchmark;


template <class DbRecord = Record<>, class DbKw = Kw> requires IsValidDbParams<DbRecord, DbKw>
class ISseServer {
public:
    // this should be the client/controller's benchmarking struct
    std::shared_ptr<Benchmark> benchmark;

    ISseServer(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void clear() = 0;
};
