#pragma once

#include <concepts>
#include <memory>

#include "utils/db.h"
#include "utils/types.h"


struct Benchmark;


template <class DbTuple = Tuple<>, class DbKw = Kw> requires IsValidDbParams<DbTuple, DbKw>
class ISseServer {
public:
    // this should be the client/controller's benchmarking struct
    std::shared_ptr<Benchmark> benchmark;

    ISseServer(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void clear() = 0;
};
