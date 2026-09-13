#pragma once

#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"


class EncIndLoc : public EncIndBase {
public:
    //--------------------------------------------------------------------------
    // rule of five

    // bring back default constructor
    EncIndLoc() = default;

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
    // `EncIndBase`

    void init(bigint bcktCount, bigint bcktSize);
    void clear() override;

private:
    bigint bcktCount = 0;
    bigint bcktSize = 0;

    //--------------------------------------------------------------------------
    // `EncIndBase`

    bigint getBcktCount() const override { return this->bcktCount; }
    bigint getBcktSize() const override { return this->bcktSize; }
};
