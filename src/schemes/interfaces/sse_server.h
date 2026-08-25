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
    ISseServer(const ISseServer& other) = delete;

    // copy assignment operator
    ISseServer& operator =(const ISseServer& other) = delete;

    // move constructor
    ISseServer(ISseServer&& other) noexcept = delete;

    // move assignment operator
    ISseServer& operator =(ISseServer&& other) noexcept = delete;

    //--------------------------------------------------------------------------
    // interface

    // this should be the client/controller's benchmarking struct
    std::shared_ptr<Benchmark> benchmark;

    ISseServer(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}

    virtual void clear() = 0;
};
