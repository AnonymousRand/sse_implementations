#pragma once

#include <concepts>
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
 *     - keywords and ids are both nonnegative integer values (implicitly convertible to `bigint`,
 *       and satisfies `std::integral`), as `DUMMY` here is used for both.
 */
static_assert(std::integral<Kw>,      "Error: `Kw` must be an integral type!");
static_assert(std::integral<Id>,      "Error: `Id` must be an integral type!");
static_assert(std::integral<IdAlias>, "Error: `IdAlias` must be an integral type!");

inline constexpr bigint DUMMY = -1;
