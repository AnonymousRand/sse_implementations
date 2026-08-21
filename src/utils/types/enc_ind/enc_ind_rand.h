#pragma once

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/ustring.h"


class EncIndRand : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    using EncIndBase::EncIndBase;

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
    // interface

    // new (non-virtual override!) versions of these methods that don't change `pos` by reference,
    // as that shouldn't be needed for pseudorandom encrypted indexes and may cause bugs later
    bool find(ubigint pos, const ustring& key, EncIndVal& ret) const {
        return EncIndBase::find(pos, key, ret);
    }

    void writeToFirstEmpty(ubigint pos, const EncIndEntry& encIndEntry) {
        EncIndBase::writeToFirstEmpty(pos, encIndEntry);
    }

private:
    //--------------------------------------------------------------------------
    // helpers

    bool advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const override;

    /**
     * returns: final entry count of `readBuf` (which may not be `readbufEntryCount` if the
     * buffer size does not divide enc ind size and there is a bit left over, for example).
     */
    bigint readIntoReadBuf(
        uchar* readBuf, bigint targetEntryCount, ubigint readBufStartPos, ubigint origStartPos,
        bool needsFseek
    ) const;
};

