#pragma once

#include <cmath>
#include <string>
#include <utility>

#include "config.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


struct Benchmark;


//==============================================================================
// utils
//==============================================================================


using EncIndVal   = std::pair<ustring, ustring>;
using EncIndEntry = std::pair<ustring, EncIndVal>;


namespace utils::enc_ind {


ustring toUstr(const EncIndEntry& encIndEntry);


} // namespace `utils::enc_ind`


//==============================================================================
// `EncIndBase`
//==============================================================================


/**
 * encrypted indexes are a collection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncIndEntry`) pairs, corresponding to `std::pair<key, std::pair<encrypted data, IV>>`.
 */
class EncIndBase : public IDiskStorage {
public:
    // (both PRF (default) and hash (res-hiding) have 512 bit output)
    inline static constexpr int KEY_LEN   = utils::crypto::HASH_OUTPUT_LEN;
    // (currently, encoding a `Tuple<>` is of the form `(id,kw,op),dbKw-dbKw` (and encrypting an
    // exactly n block length plaintext with AES should produce the exact same block ciphertext),
    // so all but 7 bytes are divided up between `id`, `kw`, and 2 `dbKw`s. however, we actually
    // must restrict our plaintexts by one more byte or else AES' PCKS #7 padding will generate
    // an extra block if our plaintext is exactly an integer number of blocks long, thus the `+8`.)
    // also round up to the next AES block
    inline static const int     DATA_LEN  =
        std::ceil((4 * config::MAX_VALUE_DIGITS + 8) / (float)utils::crypto::BLOCK_SIZE)
        * utils::crypto::BLOCK_SIZE;
    inline static constexpr int VAL_LEN   = DATA_LEN + utils::crypto::IV_LEN;
    inline static constexpr int ENTRY_LEN = KEY_LEN + VAL_LEN;

    //--------------------------------------------------------------------------
    // constructors/destructors

    EncIndBase() = default;

    //--------------------------------------------------------------------------
    // the big five

    // destructor
    ~EncIndBase() = default;

    // copy constructor
    EncIndBase(const EncIndBase& other);

    // copy assignment operator
    EncIndBase& operator =(const EncIndBase& other) = default;

    // move constructor
    EncIndBase(EncIndBase&& other) noexcept = default;

    // move assignment operator
    EncIndBase& operator =(EncIndBase&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `IDiskStorage`

    void init(bigint size);
    void clear() override;

    //--------------------------------------------------------------------------
    // other interface

    /**
     * reads and decodes the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the kv pair at `pos` is valid.
     *     - `false` if the kv pair at `pos` is the null kv pair.
     */
    bool read(ubigint pos, EncIndVal& ret) const;

    /**
     * write to `pos` (but does not check if there is already something there, e.g. from
     * `pos % this->size`, and will overwrite it!).
     */
    void write(ubigint pos, const EncIndEntry& encIndEntry, Benchmark* benchmark);

    bigint getSize() const { return this->size; }

protected:
    constexpr std::string FILE_DIR() const override { return "out/server"; }
    constexpr std::string FILENAME_PREFIX() const override { return "enc_ind_"; }

    static const uchar NULL_ENTRY[ENTRY_LEN];

    bigint size = 0;

    //--------------------------------------------------------------------------
    // helpers

    /**
     * tries to find `key` starting at `pos`, iterating forward from `pos` by `collisionSkips`
     * at a time if the key at `pos` does not match `key` (e.g. from `pos % this->size` modulo
     * collision, or if another kv pair overflowed there first).
     *
     * additionally, once `key` has been found, place the location it was found at back in `pos`
     * (e.g. in case we want to read a bucket contiguously starting from there, for locality).
     *
     * returns:
     *     - `true` if the kv pair corresponding to `key` was eventually found.
     *     - `false` if the kv pair corresponding to `key` was not found in the entire index.
     */
    bool findBase(
        ubigint& pos, const ustring& key, EncIndVal& ret,
        bigint collisionSkip, bigint collisionAttempts
    ) const;

    /**
     * write to first *empty* location at or after `pos`, iterating forward from `pos`
     * `collisionSkip` positions at a time until an empty location is found.
     *
     * additionally, once this first empty location has been found, place it back in `pos`.
     */
    void writeToFirstEmptyBase(
        ubigint& pos, const EncIndEntry& encIndEntry, Benchmark* benchmark,
        bigint collisionSkip, bigint collisionAttempts
    );

    //--------------------------------------------------------------------------
    // debugging

    EncIndEntry get(ubigint pos) const;
    void print() const; // (warning: this can be, like, a LOT of stuff!! :3)
};


//==============================================================================
// `EncIndRand`
//==============================================================================


class EncIndRand : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    EncIndRand() = default;

    //--------------------------------------------------------------------------
    // the big five

    // destructor
    ~EncIndRand() = default;

    // copy constructor
    EncIndRand(const EncIndRand& other) = default;

    // copy assignment operator
    EncIndRand& operator =(const EncIndRand& other) = default;

    // move constructor
    EncIndRand(EncIndRand&& other) noexcept = default;

    // move assignment operator
    EncIndRand& operator =(EncIndRand&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // other interface

    /**
     * tries to find `key` starting at `pos`, iterating forward *linearly* at a time
     * if the key at `pos` does not match `key` (e.g. from `pos % this->size` modulo
     * collision, or if another kv pair overflowed there first).
     *
     * this does NOT change `pos`.
     *
     * returns:
     *     - `true` if the kv pair corresponding to `key` was eventually found.
     *     - `false` if the kv pair corresponding to `key` was not found in the entire index.
     */
    bool find(ubigint pos, const ustring& key, EncIndVal& ret) const {
        // (`pos` is passed by value to `find()`, so we can pass by reference into `findBase()`
        // and still only a copy of it will be changed)
        return this->findBase(pos, key, ret, 1, this->size);
    }

    /**
     * write to first *empty* location at or after `pos`, iterating forward from `pos`
     * *linearly* until we find an empty location.
     *
     * this does NOT change `pos`.
     */
    void writeToFirstEmpty(ubigint pos, const EncIndEntry& encIndEntry, Benchmark* benchmark) {
        this->writeToFirstEmptyBase(pos, encIndEntry, benchmark, 1, this->size);
    }
};



//==============================================================================
// `EncIndLoc`
//==============================================================================


class EncIndLoc : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    EncIndLoc() = default;

    //--------------------------------------------------------------------------
    // the big five

    // destructor
    ~EncIndLoc() = default;

    // copy constructor
    EncIndLoc(const EncIndLoc& other) = default;

    // copy assignment operator
    EncIndLoc& operator =(const EncIndLoc& other) = default;

    // move constructor
    EncIndLoc(EncIndLoc&& other) noexcept = default;

    // move assignment operator
    EncIndLoc& operator =(EncIndLoc&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // other interface

    // NOTE: currently `EncIndLoc` is exactly the same as `EncIndBase`; we just still instantiate
    // it as a child class to make its semantic meaning clearer (e.g. so that we don't have to have
    // NLogN hold an `EncIndBase`, or make `EncIndRand` inherit from and hence "be an" `EncIndLoc`)
    bool find(
        ubigint& pos, const ustring& key, EncIndVal& ret,
        bigint collisionSkip, bigint collisionAttempts
    ) const {
        return this->findBase(pos, key, ret, collisionSkip, collisionAttempts);
    }

    void writeToFirstEmpty(
        ubigint& pos, const EncIndEntry& encIndEntry, Benchmark* benchmark,
        bigint collisionSkip, bigint collisionAttempts
    ) {
        this->writeToFirstEmptyBase(pos, encIndEntry, benchmark, collisionSkip, collisionAttempts);
    }
};
