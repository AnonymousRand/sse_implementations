#pragma once

#include <cstdlib>
#include <iostream>

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base.h"


class EncrIndLoc : public EncrIndBase {
public:
    //--------------------------------------------------------------------------
    // rule of five

    // bring back default constructor
    EncrIndLoc() = default;

    // destructor
    ~EncrIndLoc() = default;

    // copy constructor
    EncrIndLoc(const EncrIndLoc& other) = default;

    // copy assignment operator
    EncrIndLoc& operator =(const EncrIndLoc& other) = default;

    // move constructor
    EncrIndLoc(EncrIndLoc&& other) noexcept = default;

    // move assignment operator
    EncrIndLoc& operator =(EncrIndLoc&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `EncrIndBase`

    void init(SseOper setupOper, bigint bcktCount, bigint bcktSize);
    void clear() override;

private:
    bigint bcktCount = 0;
    bigint bcktSize = 0;

    //--------------------------------------------------------------------------
    // `EncrIndBase`

    // compared to pseudorandom encr inds, we skip buffering for advances except when bucket size
    // is 1, as otherwise we only check every bucket start pos, i.e. every `this->bcktSize` entries,
    // so buffering contiguous blocks usually becomes a waste (and more often than not,
    // we do not need to try as many positions as during setups to find the right entry)
    //
    // otherwise, we similarly only compensate for my slow implementation of advances during
    // benchmarked operations

    bool SHOULD_BUFFER_READ(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncrIndLoc::SHOULD_BUFFER_READ(): woof arf woof :3" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_WRITE(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncrIndLoc::SHOULD_BUFFER_WRITE(): woof arf woof :3" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_ADVANCE(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return this->bcktSize == 1;
        case SseOper::UPDATE: return this->bcktSize == 1;
        default:
            std::cerr << "Error: EncrIndLoc::SHOULD_BUFFER_ADVANCE(): woof arf woof :3"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bigint getBcktCount() const override { return this->bcktCount; }
    bigint getBcktSize() const override { return this->bcktSize; }
};
