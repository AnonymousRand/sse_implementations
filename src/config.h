// for experiment-specific configs/params, set them in their own files in `app/experiments/`!

#pragma once

#include <cmath>

#include "utils/crypto.h"
#include "utils/types/basic_types.h"


namespace config {


//------------------------------------------------------------------------------
// main


/**
 * whether timing should be taken after every update (and if experiments relying on that timing
 * even get run). set this to `false` if you only want to benchmark the entire `setup()` without
 * the additional computation of setting benchmarks each update (at least for non-shortcut
 * `setup()`s, i.e. those that do call `update()` for each entry).
 */
inline constexpr bool SHOULD_BENCHMARK_UPDTS = true;


/**
 * whether update benchmarking stats should be printed after every single update,
 * or only print an average at the end (as there can be very many updates!).
 */
inline constexpr bool SHOULD_PRINT_EACH_UPDT = true;


//------------------------------------------------------------------------------
// performance/shortcuts


inline constexpr bool USE_SHORTCUT_DSSE_SETUP = true;


/**
 * set this to `true` for truly large (but much slower) DBs. otherwise, DBs are stored in RAM.
 */
inline constexpr bool SHOULD_STORE_DBS_ON_DISK = false;


/**
 * the capacity in # of entries for the (non-locality) encrypted index read buffers (which help
 * speed up massive operations, mainly setups, by somewhat compensating for my slow implementation
 * of what happens if two entries are written to the same position after a modulo).
 *
 * search and update buffers are recommended to be smaller than the setup buffer, both since there
 * is usually less need for buffering and also so on-disk performance is more accurately measured.
 */
// currently: an enc ind entry is 96 bytes (48 tuple + 16 iv + 32 label/hash), so 2^26 => ~6.44 GB,
// which should accommodate Log-SRC-i[PiBas/NLogN] up to 2^20 and Log-SRC-i* up to 2^19
// (noting that NLogN maintains a separate enc ind instance and hence buffer for each level;
// HOWEVER since all instances are active simultaneously, you may still run out of RAM past 2^15)
inline constexpr bigint ENC_IND_SETUP_BUF_CAPAC = std::pow(2, 26);
// this is for when the enc ind does not fit entirely in memory; i recommend this being
// quite a bit smaller than the main setup buf as there will be lots of filling and flushing
inline constexpr bigint ENC_IND_SETUP_OVERFLOW_BUF_CAPAC = std::pow(2, 8);
// search buf size is determined heuristically from enc ind size (currently: 1/2^9 of enc ind size);
// this is its maximum allowed size
inline constexpr bigint ENC_IND_SEARCH_BUF_MAX_CAPAC = ENC_IND_SETUP_BUF_CAPAC / std::pow(2, 9);
inline constexpr bigint ENC_IND_UPDATE_BUF_CAPAC = std::pow(2, 8);
static_assert(
    ENC_IND_SETUP_BUF_CAPAC > 0,
    "Error: `ENC_IND_SETUP_BUF_CAPAC` must be strictly positive!"
);
static_assert(
    ENC_IND_SETUP_OVERFLOW_BUF_CAPAC > 0,
    "Error: `ENC_IND_SETUP_OVERFLOW_BUF_CAPAC` must be strictly positive!"
);
static_assert(
    ENC_IND_SEARCH_BUF_MAX_CAPAC > 0,
    "Error: `ENC_IND_SEARCH_BUF_MAX_CAPAC` must be strictly positive!"
);
static_assert(
    ENC_IND_UPDATE_BUF_CAPAC > 0,
    "Error: `ENC_IND_UPDATE_BUF_CAPAC` must be strictly positive!"
);


//------------------------------------------------------------------------------
// other


inline constexpr int ENCODED_NUMBER_BITS = 29;

/**
 * the max number of decimal digits you want ids and keywords to be able to support
 * (this determines the size of each entry in encrypted indexes; see `TUPLE_ENCOD_LEN` below).
 */
// currently: 11 is the largest possible value such that each encrypted tuple fits in
// 3 AES blocks (= 48 bytes)
inline constexpr int MAX_VALUE_DIGITS = 11;
static_assert(
    MAX_VALUE_DIGITS > 0, "Error: `MAX_VALUE_DIGITS` must be strictly positive!"
);

// currently, encoding a `Tuple<>` is of the form `id,kw[op]dbKw-dbKw`, so all but 3 bytes are
// divided up between `id`, `kw`, and 2 `dbKw`s. however, we actually must restrict our plaintexts
// by one more byte or else AES' PCKS #7 padding will generate an extra block if our plaintext is
// exactly block-aligned, thus the `+ 4`. also, `SrcIDb1Tuple`s have the same max length encoding.
// IMPORTANT: update if encoding changes!
inline constexpr int TUPLE_ENCOD_LEN =
    std::ceil((4 * MAX_VALUE_DIGITS + 4) / (float)utils::crypto::BLOCK_SIZE)
    * utils::crypto::BLOCK_SIZE;


} // namespace `config`
