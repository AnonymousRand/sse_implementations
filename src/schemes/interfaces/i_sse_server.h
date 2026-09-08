#pragma once

#include <concepts>

#include "types/basic_types.h"
#include "types/tuple.h"


template <IsDbTuple DbTuple = Tuple<>>
class ISseServer {
public:
    //--------------------------------------------------------------------------
    // rule of five

    // delete all these to prevent copying and moving! as they cause double frees
    // and all that yummy stuff with raw pointer members
    // IMPORTANT: this means SSE server classes can only be instantiated as pointers!

    // bring back default constructor
    ISseServer() = default;

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

    virtual void clear() = 0;
};
