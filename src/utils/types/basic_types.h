#pragma once

#include <concepts>
#include <cstdint>
#include <format>

#include "config.h"


// generic "long" types
using bigint  = std::int64_t;
using ubigint = std::uint64_t;


using Kw      = bigint;
using Id      = bigint;
using IdAlias = bigint; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)
// these asserts are important for encoding!
static_assert(
    sizeof(Kw) >= config::INT_MAX_BYTES,
    std::format(
        "Error: `Kw` must be at least `config::INT_MAX_BYTES` ({}) bytes, it's currently {}",
        config::INT_MAX_BYTES, sizeof(Kw)
    )
)
static_assert(
    sizeof(Id) >= config::INT_MAX_BYTES,
    std::format(
        "Error: `Id` must be at least `config::INT_MAX_BYTES` ({}) bytes, it's currently {}",
        config::INT_MAX_BYTES, sizeof(Id)
    )
)
static_assert(
    sizeof(IdAlias) >= config::INT_MAX_BYTES,
    std::format(
        "Error: `IdAlias` must be at least `config::INT_MAX_BYTES` ({}) bytes, it's currently {}",
        config::INT_MAX_BYTES, sizeof(IdAlias)
    )
)


enum class Op : char {
    INS   = 'I',
    DEL   = 'D',
    DUMMY = 'X'
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


enum class SseOper {
    SETUP,
    SEARCH,
    UPDATE
};
