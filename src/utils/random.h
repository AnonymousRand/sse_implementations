#pragma once

#include <random>


namespace {


std::random_device RAND_DEV;


} // anonymous namespace


//==============================================================================
// random utils
//==============================================================================


namespace utils {


inline std::mt19937 RNG(RAND_DEV());


} // namespace `utils`
