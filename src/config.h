// for experiment-specific configs/params, set them in their own files in `app/experiments/`!

#pragma once

#include <cmath>
#include <string>

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
// currently: an encr ind entry is 80 bytes (32 tuple + 16 iv + 32 label/hash), so 2^26 => ~5.37 GB,
// which should accommodate Log-SRC-i[PiBas/NLogN] up to 2^20 and Log-SRC-i* up to 2^19
// (noting that NLogN maintains a separate encr ind instance and hence buffer for each level;
// HOWEVER since all instances are active simultaneously, you may still run out of RAM past 2^15)
inline constexpr bigint ENC_IND_SETUP_BUF_CAPAC = std::pow(2, 26);
// this is for when the encr ind does not fit entirely in memory; i recommend this being
// quite a bit smaller than the main setup buf as there will be lots of filling and flushing
inline constexpr bigint ENC_IND_SETUP_OVERFLOW_BUF_CAPAC = std::pow(2, 8);
// search buf size is determined heuristically from encr ind size (currently: 1/2^9 of
// encr ind size); this is its maximum allowed size
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


/**
 * the max number of bytes you want ids and keywords to be able to take up
 * (this determines the size of each entry in encrypted indexes; see `TUPLE_ENCOD_LEN` below).
 */
// currently: 4 is the largest possible value such that each encrypted tuple fits in
// 2 AES blocks (= 32 bytes), and corresponds to DB sizes/highest IDs up to 2^32
inline constexpr int INT_MAX_BYTES = 4;
static_assert(INT_MAX_BYTES > 0, "Error: `INT_MAX_BYTES` must be strictly positive!");
// these asserts are important for encoding! (can't do in `basic_types.h` since circular import)
static_assert(sizeof(Id) >= INT_MAX_BYTES, "Error: `Id` must be at least `INT_MAX_BYTES` bytes");
static_assert(sizeof(Kw) >= INT_MAX_BYTES, "Error: `Kw` must be at least `INT_MAX_BYTES` bytes");
static_assert(
    sizeof(IdAlias) >= INT_MAX_BYTES, "Error: `IdAlias` must be at least `INT_MAX_BYTES` bytes"
);

// currently, encoding a `Tuple<>` is of the form `id|kw|[op]|dbKw|dbKw` while an `SrcIDb1Doc` is
// `kw|id'|id'|kw|kw`, meaning the latter is the larger encoding, consisting of 5 "numbers".
// thus, we multiply `INT_MAX_BYTES` by 5 to obtain the total max size of an encoded tuple.
// however, we actually must restrict our plaintexts by one more byte or else AES' PCKS #7
// padding will generate an extra block if our plaintext is exactly block-aligned, thus the `+ 1`.
// IMPORTANT: update if encoding changes!
inline constexpr int TUPLE_ENCOD_LEN =
    std::ceil((5 * INT_MAX_BYTES + 1) / (float)utils::crypto::BLOCK_SIZE)
    * utils::crypto::BLOCK_SIZE;


} // namespace `config`
