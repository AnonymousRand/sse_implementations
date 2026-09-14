#pragma once

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/enc_ind/enc_ind_types.h"
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

    // new (non-virtual shadow!) versions of these methods that don't change `pos` by reference,
    // as that shouldn't be needed for pseudorandom encrypted indexes and may cause bugs later
    bool find(Oper oper, ubigint pos, const ustring& key, EncIndVal& ret) const {
        return EncIndBase::find(oper, pos, key, ret);
    }

    void writeToFirstEmpty(Oper oper, ubigint pos, const EncIndEntry& encIndEntry) {
        EncIndBase::writeToFirstEmpty(oper, pos, encIndEntry);
    }

private:
    //--------------------------------------------------------------------------
    // `EncIndBase`

    const bool SHOULD_BUFFER_READ(Oper oper) const override { return true; }
    const bool SHOULD_BUFFER_WRITE(Oper oper) const override { return true; }

    // this essentially means we have no buckets; each individual entry is a "bucket"
    bigint getBcktSize() const override { return 1; }
    bigint getBcktCount() const override { return this->capacity; }
};
