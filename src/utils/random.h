#pragma once

#include <random>


static std::random_device RAND_DEV;
static std::mt19937 RNG(RAND_DEV());
