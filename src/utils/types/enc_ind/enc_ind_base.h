#pragma once

#include <string>

#include "config.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


class EncIndBase : public IDiskStorage {
public:
    // (currently, all schemes are result-hiding, which uses a hash as the final key here)
    // IMPORTANT: change if this is no longer the case!
    virtual constexpr int KEY_LEN() const { return utils::crypto::HASH_OUTPUT_LEN; }
    // (we can use the encoded (plaintext) tuple length here for encrypted tuples too, as encrypting
    // an exactly `n`-block-length plaintext with AES-CBC produces a ciphertext of the same size)
    // IMPORTANT: change if this is no longer the case!
    virtual constexpr int DATA_LEN() const { return config::TUPLE_ENCOD_LEN; }
    constexpr int VAL_LEN() const { return this->DATA_LEN() + utils::crypto::IV_LEN; }
    constexpr int ENTRY_LEN() const { return this->KEY_LEN() + this->VAL_LEN(); }

    //--------------------------------------------------------------------------
    // constructors/destructors

    virtual ~EncIndBase();

    //--------------------------------------------------------------------------
    // rule of five

    // bring back default constructor
    EncIndBase() = default;

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
     * read and decode the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool read(ubigint pos, EncIndVal& ret, bool shouldFseek = true) const;

    /**
     * try to find `key` starting at `pos`, iterating forward from `pos` if the key
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
    void write(ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek = true);

    /**
     * write to first *empty* location at or after `pos`, iterating forward from `pos` until
     * an empty location is found.
     *
     * returns in `pos`: this final empty location (in case you may need it for e.g.
     * contiguous writing of a locality-aware bucket after determining its start position).
     */
    void writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry);

    // (mostly for debugging)
    void print() const; // (warning: this can be, like, a LOT of stuff!! :3)

    bigint getCapacity() const { return this->capacity; }
    bigint getBytes() const { return this->capacity * this->ENTRY_LEN(); }

protected:
    uchar* NULL_ENTRY = nullptr;
    bigint capacity = 0;

    //--------------------------------------------------------------------------
    // `IDiskStorage`

    constexpr std::string FILE_DIR() const override { return "out/server"; }
    constexpr std::string FILENAME_PREFIX() const override { return "enc_ind_"; }

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

    /**
     * read and decode the *entry* (not just the value, i.e. including the key) at `pos`.
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool readEntry(ubigint pos, EncIndEntry& ret) const;
    void readEncoded(uchar* buf) const;

    void writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek = true);
};
