#pragma once

#include <cstdlib>
#include <iostream>

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

    void init(SseOper setupOper, bigint bcktSize, bigint bcktCount);
    void clear() override;

private:
    bigint bcktSize = 0;
    bigint bcktCount = 0;

    //--------------------------------------------------------------------------
    // `EncIndBase`

    // we skip buffering for searches for locality-aware enc inds except when bucket size is 1,
    // as otherwise we only check every bucket start pos, i.e. every `this->bcktSize` entries,
    // so buffering contiguous blocks usually becomes a waste (and more often than not, we
    // do not need to try as many positions as during setups to find the right entry)
    bool SHOULD_BUFFER_READ(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:
            return true;
        case SseOper::SEARCH:
            return this->bcktSize == 1;
        case SseOper::UPDATE:
            return true;
        default:
            std::cerr << "Error: EncIndLoc::SHOULD_BUFFER_READ(): mama wee zoo" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_WRITE(SseOper oper) const override { return true; }

    bigint getBcktSize() const override { return this->bcktSize; }
    bigint getBcktCount() const override { return this->bcktCount; }
};
