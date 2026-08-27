#pragma once

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/ustring.h"


class EncIndLoc : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    using EncIndBase::EncIndBase;

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
    // interface

    void init(bigint bcktSize, bigint bcktCount);
    void clear() override;

private:
    bigint bcktSize = 0;
    bigint bcktCount = 0;

    //--------------------------------------------------------------------------
    // `EncIndBase`

    bool advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const override;
};
