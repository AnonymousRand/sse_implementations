#pragma once

#include <concepts>
#include <memory>

#include "utils/tuple.h"
#include "utils/types.h"


struct Benchmark;


template <IsDbTuple DbTuple = Tuple<>>
class ISseServer {
public:
    // this should be the client/controller's benchmarking struct
    std::shared_ptr<Benchmark> benchmark;

    ISseServer(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void clear() = 0;
};
