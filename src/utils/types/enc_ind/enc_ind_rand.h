#pragma once

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/ustring.h"


class EncIndRand : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // rule of five

    // bring back default constructor
    EncIndRand() = default;

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
    // interface

    void init(bigint capacity) override;
    void clear() override;

    // new (non-virtual shadow!) versions of these methods that don't change `pos` by reference,
    // as that shouldn't be needed for pseudorandom encrypted indexes and may cause bugs later
    bool find(ubigint pos, const ustring& key, EncIndVal& ret) const {
        return EncIndBase::find(pos, key, ret);
    }

    void writeToFirstEmpty(ubigint pos, const EncIndEntry& encIndEntry) {
        EncIndBase::writeToFirstEmpty(pos, encIndEntry);
    }

private:
    struct Buf {
        static const bigint INVALID_INDEX;

        uchar* data = nullptr;
        const bigint ENTRY_CAPACITY;
        ubigint startPos = 0;
        ubigint endPos = 0;
        bigint entryCount = 0;

        Buf(bigint ENTRY_CAPACITY, bigint entryLen);
        ~Buf();
    };

    mutable Buf* buf = nullptr;

    //--------------------------------------------------------------------------
    // `EncIndBase`

    bool advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const override;

    void writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek) override;

    //--------------------------------------------------------------------------
    // helpers

    void readIntoBuf(ubigint bufStartPos, ubigint origStartPos, bool needsFseek) const;
    void flushBufToFile();
    bigint posToBufIndex(ubigint pos) const;
};
