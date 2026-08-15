#pragma once

#include <cstdint>


// generic "long" types
using bigint  = std::int64_t;
using ubigint = std::uint64_t;


using Kw      = bigint;
using Id      = bigint;
using IdAlias = bigint; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)


enum class Op : char {
    INS   = 'I',
    DEL   = 'D',
    DUMMY = '-'
};


/**
 * preconditions:
 *     - keywords and ids are both nonnegative integer values (storable by `bigint`)
 *       (as `DUMMY` here is used for both).
 */
inline constexpr bigint DUMMY = -1;
