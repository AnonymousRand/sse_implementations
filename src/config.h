#pragma once


namespace config {


inline constexpr bool SHOULD_BENCHMARK            = true;
inline constexpr bool DSSE_USE_SHORTCUT_SETUP     = true;
inline constexpr bool DSSE_SHOULD_BENCHMARK_UPDTS = true;


// this is the max number of decimal digits you want ids and keywords to be able to support
// this is used to determine the size of each entry in encrypted indexes (see that file for details)
// currently: 14 corresponds to each encrypted tuple taking 4 AES blocks
inline constexpr int MAX_VALUE_DIGITS = 14;


// set this to `true` for truly large (but much slower) DBs; otherwise DBs are stored in RAM
inline constexpr bool SHOULD_STORE_DBS_ON_DISK = true;


} // namespace `config`
