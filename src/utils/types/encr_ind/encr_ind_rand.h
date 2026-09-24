#pragma once

#include <cstdlib>
#include <iostream>

#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/ustring.h"


class EncrIndRand : public EncrIndBase {
public:
    //--------------------------------------------------------------------------
    // rule of five

    // bring back default constructor
    EncrIndRand() = default;

    // destructor
    ~EncrIndRand() = default;

    // copy constructor
    EncrIndRand(const EncrIndRand& other) = default;

    // copy assignment operator
    EncrIndRand& operator =(const EncrIndRand& other) = default;

    // move constructor
    EncrIndRand(EncrIndRand&& other) noexcept = default;

    // move assignment operator
    EncrIndRand& operator =(EncrIndRand&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // interface

    // new (non-virtual shadow!) versions of these methods that don't change `pos` by reference,
    // as that shouldn't be needed for pseudorandom encrypted indexes and may cause bugs later
    bool find(SseOper oper, ubigint pos, const ustring& key, EncrIndVal& ret) const {
        return EncrIndBase::find(oper, pos, key, ret);
    }

    void writeToFirstEmpty(SseOper oper, ubigint pos, const EncrIndEntry& encIndEntry) {
        EncrIndBase::writeToFirstEmpty(oper, pos, encIndEntry);
    }

private:
    //--------------------------------------------------------------------------
    // `EncrIndBase`

    // we don't buffer reads or writes during benchmarked operations (search, update) except
    // during advances, keeping in line with the buffer's purpose of only compensating for
    // my slow implementation of advances during benchmarked operations

    bool SHOULD_BUFFER_READ(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncrIndRand::SHOULD_BUFFER_READ(): mrrp :3" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_WRITE(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncrIndRand::SHOULD_BUFFER_WRITE(): mrrp :3" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_ADVANCE(SseOper oper) const override { return true; }

    // this essentially means we have no buckets; each individual entry is a "bucket"
    bigint getBcktCount() const override { return this->capacity; }
    bigint getBcktSize() const override { return 1; }
};
