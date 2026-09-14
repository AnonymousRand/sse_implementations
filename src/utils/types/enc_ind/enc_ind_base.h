#pragma once

#include <concepts>
#include <cstdlib>
#include <functional>
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

    // forward declaration
    enum class BufType;

    virtual void init(bigint capacity);
    void clear() override;

    /**
     * read and decode the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool read(BufType bufType, ubigint pos, EncIndVal& ret) const;

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
    void write(BufType bufType, ubigint pos, const EncIndEntry& encIndEntry);

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
    bool advanceUntilMatch(BufType bufType, ubigint& pos, const uchar* match, int matchLen) const;

    /**
     * the raw read and write methods. these should be the ONLY read/write methods that touch
     * the buffers or the file!
     *
     * returns: a pointer to the start of the *buffer* location where the entry is.
     * IMPORTANT: this points to the same memory as the buffer data does (i.e. no `memcpy()`s),
     * so do NOT allocate any new memory to hold it or free the returned value in the caller!!
     */
    uchar* readEncoded(BufType bufType, ubigint pos) const;
    void readEncodedNoBuf(ubigint pos, uchar* ret, bool shouldFseek = true) const;
    void writeEncoded(BufType bufType, ubigint pos, const uchar* encodedEntry, bool isInit = false);

    /**
     * read and decode the *entry* (not just the value, i.e. including the key) at `pos`.
     *
     * returns:
     *     - `true` if the entry at `pos` is valid.
     *     - `false` if the entry at `pos` is the null entry.
     */
    bool readEntry(BufType bufType, ubigint pos, EncIndEntry& ret) const;


//==============================================================================
// `EncIndBase::Buf`
//==============================================================================


    struct Buf {
    public:
        // mainly for debugging/assertions
        friend class EncIndBase;

        static const bigint NOT_IN_BUF;

        //----------------------------------------------------------------------
        // constructors/destructors

        Buf(
            bigint ENTRY_CAPACITY,
            FILE* file, const std::string& filename, bigint encIndCapacity, bigint entryLen
        );

        ~Buf();

        //----------------------------------------------------------------------
        // rule of five

        // all but copy constructor deleted since copy constructor should be the only one we need

        // copy constructor
        Buf(const Buf& other);

        // copy assignment operator
        Buf& operator =(const Buf& other) = delete;

        // move constructor
        Buf(Buf&& other) noexcept = delete;

        // move assignment operator
        Buf& operator =(Buf&& other) noexcept = delete;

        //----------------------------------------------------------------------
        // interface

        /**
         * returns: a pointer to the start of the *buffer* location where the entry is.
         * IMPORTANT: this points to the same memory as the buffer data does (i.e. no `memcpy()`s),
         * so do NOT allocate any new memory to hold it or free the returned value in the caller!!
         */
        uchar* read(bigint index) const;

        /**
         * note: this *does* `memcpy()` the data from `entry` into the buffer.
         */
        void write(bigint index, const uchar* entry);

        void fill(ubigint startPos, bool allowIncompleteFill = false);
        void flushIfNotFlushed() const;

        bigint posToBufIndex(ubigint pos) const;

    private:
        const bigint ENTRY_CAPACITY;
        uchar* data = nullptr;
        ubigint startPos = 0;
        ubigint endPos = 0;
        bool isFilled = false;
        mutable bool isFlushed = true;

        // members shared with its parent enc ind (do not free these in `Buf`!!)
        FILE* file;
        const std::string& filename;
        const bigint encIndCapacity;
        const bigint entryLen;

        /**
         * helper for sharing code between `fill()` and `flush()`. (the template and the `static`
         * are needed for this to be usable in both the non-const `fill()` and the const `flush()`.)
         *
         * params:
         *     - `isRead`: set to `true` for reads and `false` for writes.
         */
        template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, Buf>
        static void operOnFileBase(
            SelfType* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
        );
    };

public:
    // public-facing interface methods should use `BufType` as parameters to hide the
    // `Buf*` members, while internal ones can use `Buf*` (hence why `BufType` is `public`)
    enum class BufType {
        SETUP,
        SEARCH
    };

protected:
    mutable Buf* setupBuf = nullptr;
    mutable Buf* searchBuf = nullptr;

    /**
     * translate public-facing `BufType` to a `Buf*` member.
     */
    Buf* getBufToUse(BufType bufType) const {
        switch (bufType) {
        case BufType::SETUP:
            return this->setupBuf;
        case BufType::SEARCH:
            return this->searchBuf;
        default:
            std::cerr << "Error: EncIndBase::getBufToUse(): zoo wee mama" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    //--------------------------------------------------------------------------
    // `EncIndBase` helpers

    void fillBuf(Buf* buf, ubigint bufStartPos, bool isEncIndInit = false) const;
    void flushBufIfNotFlushed(Buf* buf) const;

    bigint posToBufIndex(Buf* buf, ubigint pos) const;
};
