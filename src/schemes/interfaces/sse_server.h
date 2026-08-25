#pragma once

#include <concepts>
#include <memory>

#include "utils/types/basic_types.h"
#include "utils/types/tuple.h"


struct Benchmark;


template <IsDbTuple DbTuple = Tuple<>>
class ISseServer {
public:
    //--------------------------------------------------------------------------
    // the big five

    // delete all these to prevent copying and moving! as they cause double frees
    // and all that yummy stuff with raw pointer members
    // IMPORTANT: this means SSE server classes can only be instantiated as pointers!

    // copy constructor
    ISse(const ISse& other) = delete;

    // copy assignment operator
    ISse& operator =(const ISse& other) = delete;

    // move constructor
    ISse(ISse&& other) noexcept = delete;

    // move assignment operator
    ISse& operator =(ISse&& other) noexcept = delete;
    // this should be the client/controller's benchmarking struct
    std::shared_ptr<Benchmark> benchmark;

    //--------------------------------------------------------------------------
    // interface

    ISseServer(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void clear() = 0;
};
