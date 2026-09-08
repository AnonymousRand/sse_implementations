// for experiment-specific configs/params, set them in their own files in `app/experiments/`!

#pragma once

#include <cmath>

#include "types/basic_types.h"

#include "utils/crypto.h"


namespace config {


//------------------------------------------------------------------------------
// benchmarking


inline constexpr bool SHOULD_BENCHMARK       = true;
inline constexpr bool SHOULD_BENCHMARK_UPDTS = true;


//------------------------------------------------------------------------------
// performance/shortcuts


inline constexpr bool USE_SHORTCUT_DSSE_SETUP = true;

// set this to `true` for truly large (but much slower) DBs
// otherwise DBs are stored in RAM
inline constexpr bool SHOULD_STORE_DBS_ON_DISK = false;

// this is the capacity in # of entries for the (non-locality) encrypted index read buffers,
// which help speed up massive `setup()` calls
// i find that 2^8 is a pretty good balance
inline constexpr bigint ENC_IND_READ_BUF_CAPACITY = std::pow(2, 8);
static_assert(
    ENC_IND_READ_BUF_CAPACITY > 0, "Error: `ENC_IND_READ_BUF_CAPACITY` must be strictly positive!"
);


//------------------------------------------------------------------------------
// other


// this is the max number of decimal digits you want ids and keywords to be able to support
// this is used to determine the size of each entry in encrypted indexes (see that file for details)
// currently: 11 corresponds to each encrypted tuple taking 3 AES blocks (= 48 bytes)
inline constexpr int MAX_VALUE_DIGITS = 11;

// currently, encoding a `Tuple<>` is of the form `id,kw[op]dbKw-dbKw`, so all but 3 bytes are
// divided up between `id`, `kw`, and 2 `dbKw`s. however, we actually must restrict our plaintexts
// by one more byte or else AES' PCKS #7 padding will generate an extra block if our plaintext is
// exactly block-aligned, thus the `+ 4`. also, `SrcIDb1Tuple`s have the same max length encoding.
inline constexpr int TUPLE_ENCOD_LEN =
    std::ceil((4 * MAX_VALUE_DIGITS + 4) / (float)utils::crypto::BLOCK_SIZE)
    * utils::crypto::BLOCK_SIZE;


} // namespace `config`
