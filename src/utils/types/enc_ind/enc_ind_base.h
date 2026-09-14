#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#include "config.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


//==============================================================================
// `EncIndBase`
//==============================================================================


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

protected:
    // (non-virtually) redeclaring these completely to also take into account new member variables
    // (non-virtual since virtual polymorphism doesn't work anyway in the base class' constructors)
    void copyFrom(const EncIndBase& other);
    void moveFrom(EncIndBase&& other) noexcept;

public:
    // bring back default constructor
    EncIndBase() = default;

    // copy constructor
    EncIndBase(const EncIndBase& other);

    // copy assignment operator
    EncIndBase& operator =(const EncIndBase& other);

    // move constructor
    EncIndBase(EncIndBase&& other) noexcept;

    // move assignment operator
    EncIndBase& operator =(EncIndBase&& other) noexcept;

    //--------------------------------------------------------------------------
    // interface

    // public-facing interface methods should use `Oper` as parameters to hide the
    // `Buf*` members, while internal ones can use `Buf*` (hence why `Oper` is `public`)
    enum class Oper {
        SETUP,
        SEARCH
    };

    virtual void init(bigint capacity);
    void clear() override;

    /**
     * read and decode the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool read(Oper oper, ubigint pos, EncIndVal& ret, bool shouldFseek = true) const;

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
    bool find(Oper oper, ubigint& pos, const ustring& key, EncIndVal& ret) const;

    /**
     * write to `pos` (but does not check if there is already something there, e.g. from
     * `pos % this->capacity`, and will overwrite it!).
     */
    void write(Oper oper, ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek = true);

    /**
     * write to first *empty* location at or after `pos`, iterating forward from `pos` until
     * an empty location is found.
     *
     * returns in `pos`: this final empty location (in case you may need it for e.g.
     * contiguous writing of a locality-aware bucket after determining its start position).
     */
    void writeToFirstEmpty(Oper oper, ubigint& pos, const EncIndEntry& encIndEntry);

    /**
     * this method MUST be called when all setup operations done! e.g. they flush the buffers,
     * since different operations currently use different buffers.
     *
     * IMPORTANT: this also means that enc inds must have all setup operations performed before
     * all search operations, as otherwise syncing the 2 buffers becomes quite a nightmare.
     *
     * (i could let enc inds track and handle this automatically, but that can only be done
     * upon starting the new oper, which would then impact benchmarking (e.g. flushing
     * a huge setup buffer at the start of a search.)
     */
    void endSetup();

    bigint getCapacity() const { return this->capacity; }
    bigint getBytes() const { return this->capacity * this->ENTRY_LEN(); }

    // (mostly for debugging)
    void print() const; // (warning: this can be, like, a LOT of stuff!! :3)

protected:
    uchar* NULL_ENTRY = nullptr;
    bigint capacity = 0;

    virtual const bool SHOULD_BUFFER_READ(Oper oper) const = 0;
    virtual const bool SHOULD_BUFFER_WRITE(Oper oper) const = 0;

    virtual bigint getBcktSize() const = 0;
    virtual bigint getBcktCount() const = 0;

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
    bool advanceUntilMatch(Oper oper, ubigint& pos, const uchar* match, int matchLen) const;

    /**
     * the raw read and write methods. these should be the ONLY read/write methods that touch
     * the buffers or the file!
     *
     * returns: a pointer to the start of the *buffer* location where the entry is.
     * IMPORTANT: this points to the same memory as the buffer data does (i.e. no `memcpy()`s),
     * so do NOT allocate any new memory to hold it or free the returned value in the caller!!
     */
    uchar* readEncoded(Oper oper, ubigint pos) const;
    void writeEncoded(Oper oper, ubigint pos, const uchar* encodedEntry, bool isInit = false);

    /**
     * the same as `readEncoded()`/`writeEncoded()`, but not filling up the buffer and reading/
     * writing directly from/to the file instead if the requested `pos` is not within the buffer.
     *
     * IMPORTANT: these should still guarantee that if the requested entry is in the buffer, the
     * read/write still happens in the buffer instead of in the file, as the buffer must hold the
     * more up-to-date version of the entries it contains. this should ensure that it is ALWAYS
     * correct to read from the buffer.
     */
    void readEncodedNoBuf(Oper oper, ubigint pos, uchar* ret, bool shouldFseek = true) const;
    void writeEncodedNoBuf(
        Oper oper, ubigint pos, const uchar* encodedEntry, bool shouldFseek = true
    );

    /**
     * read and decode the *entry* (not just the value, i.e. including the key) at `pos`.
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool readEntry(Oper oper, ubigint pos, EncIndEntry& ret, bool shouldFseek = true) const;

    //--------------------------------------------------------------------------
    // buffer

    // forward declare; `Buf` is a nested class declared in another file
    struct Buf;

    mutable Buf* setupBuf = nullptr;
    mutable Buf* searchBuf = nullptr;

    /**
     * translate public-facing `Oper` to a `Buf*` member.
     */
    Buf* getBufFromOper(Oper oper) const {
        switch (oper) {
        case Oper::SETUP:
            return this->setupBuf;
        case Oper::SEARCH:
            return this->searchBuf;
        default:
            std::cerr << "Error: EncIndBase::getBufFromOper(): zoo wee mama" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    void fillBuf(Buf* buf, ubigint bufStartPos, bool isEncIndInit = false) const;
    void flushBufIfNotFlushed(Buf* buf) const;

    bigint posToBufIndex(Buf* buf, ubigint pos) const;
};
