#pragma once

#include <cmath>
#include <memory>
#include <string>

#include "config.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


// still need to forward declare here to avoid some circular dependency
struct Benchmark;


class EncIndBase : public IDiskStorage {
public:
    // (both PRF (default) and hash (res-hiding) have 512 bit output)
    inline static constexpr int KEY_LEN   = utils::crypto::HASH_OUTPUT_LEN;
    // (currently, encoding a `Tuple<>` is of the form `id,kw[op]dbKw-dbKw`, and encrypting an
    // exactly n block length plaintext with AES should produce the exact same block ciphertext,
    // so all but 3 bytes are divided up between `id`, `kw`, and 2 `dbKw`s. however, we actually
    // must restrict our plaintexts by one more byte or else AES' PCKS #7 padding will generate
    // an extra block if our plaintext is exactly an integer number of blocks long, thus the `+8`.
    // we also round up to the next AES block. and also, `SrcIDb1Tuple`s have the same max length.)
    inline static const int     DATA_LEN  =
        std::ceil((4 * config::MAX_VALUE_DIGITS + 3) / (float)utils::crypto::BLOCK_SIZE)
        * utils::crypto::BLOCK_SIZE;
    inline static constexpr int VAL_LEN   = DATA_LEN + utils::crypto::IV_LEN;
    inline static constexpr int ENTRY_LEN = KEY_LEN + VAL_LEN;

    //--------------------------------------------------------------------------
    // constructors/destructors

    EncIndBase(std::shared_ptr<Benchmark> benchmark);

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
    // interface

    virtual void init(bigint capacity);
    void clear() override;

    /**
     * reads and decodes the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool read(ubigint pos, EncIndVal& ret) const;

    /**
     * tries to find `key` starting at `pos`, iterating forward from `pos` if the key
     * at `pos` does not match `key` (e.g. if another entry overflowed there first).
     *
     * returns in `pos`: the location at which `key` was found (in case you may need it for e.g.
     * contiguous reading of a locality-aware bucket after determining its start position).
     *
     * returns:
     *     - `true` if the entry corresponding to `key` was found.
     *     - `false` if the entry corresponding to `key` was not found in the entire index.
     */
    bool find(ubigint& pos, const ustring& key, EncIndVal& ret) const;

    /**
     * write to `pos` (but does not check if there is already something there, e.g. from
     * `pos % this->capacity`, and will overwrite it!).
     */
    void write(ubigint pos, const EncIndEntry& encIndEntry);

    /**
     * write to first *empty* location at or after `pos`, iterating forward from `pos` until
     * an empty location is found.
     *
     * returns in `pos`: this final empty location (in case you may need it for e.g.
     * contiguous writing of a locality-aware bucket after determining its start position).
     */
    void writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry);

    bigint getCapacity() const { return this->capacity; }

protected:
    constexpr std::string FILE_DIR() const override { return "out/server"; }
    constexpr std::string FILENAME_PREFIX() const override { return "enc_ind_"; }

    static const uchar NULL_ENTRY[ENTRY_LEN];

    bigint capacity = 0;
    std::shared_ptr<Benchmark> benchmark;

    //--------------------------------------------------------------------------
    // helpers

    /**
     * advance forward from `pos` until the first `matchLen` bytes of the current entry
     * matches `match`, or we've traversed the entire index.
     *
     * returned in `pos`: the final location that matched `match` (if we found one).
     *
     * returns:
     *     - `true` if an entry matching `match` was found.
     *     - `false` if an entry matching `match` was found was not found in the entire index.
     */
    virtual bool advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const = 0;

    void readEncoded(uchar* buf) const;

    /**
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool readEncIndEntry(ubigint pos, EncIndEntry& ret) const;

    void writeEncoded(ubigint pos, const uchar* encodedEntry);

    // (mostly for debugging)
    void print() const; // (warning: this can be, like, a LOT of stuff!! :3)
};
