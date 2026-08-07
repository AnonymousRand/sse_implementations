#pragma once

#include <cstdint>
#include <random>

#include <openssl/evp.h>


// lengths are in bytes
static constexpr int KEY_LEN         = 256 / 8;
static constexpr int IV_LEN          = 128 / 8;
static constexpr int BLOCK_SIZE      = 128 / 8;
static constexpr int HASH_OUTPUT_LEN = 512 / 8;
static const EVP_CIPHER* ENC_CIPHER  = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC       = EVP_sha512();

/**
 * preconditions:
 *     - keywords and ids are both nonnegative integral values (storable by `int64_t`)
 *       (as `DUMMY` is used for both).
 */
static constexpr int64_t DUMMY = -1;


static std::random_device RAND_DEV;
static std::mt19937 RNG(RAND_DEV());
