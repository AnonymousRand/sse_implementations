#pragma once

#include <cstdint>


using Kw      = int64_t;
using Id      = int64_t;
using IdAlias = int64_t; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)


enum class Op : char {
    INS   = 'I',
    DEL   = 'D',
    DUMMY = '-'
};


/**
 * preconditions:
 *     - keywords and ids are both nonnegative integer values (storable by `int64_t`)
 *       (as `DUMMY` here is used for both).
 */
inline constexpr int64_t DUMMY = -1;
