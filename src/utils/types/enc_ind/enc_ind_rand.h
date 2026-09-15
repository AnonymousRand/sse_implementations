#pragma once

#include <cstdlib>
#include <iostream>

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
    bool find(SseOper oper, ubigint pos, const ustring& key, EncIndVal& ret) const {
        return EncIndBase::find(oper, pos, key, ret);
    }

    void writeToFirstEmpty(SseOper oper, ubigint pos, const EncIndEntry& encIndEntry) {
        EncIndBase::writeToFirstEmpty(oper, pos, encIndEntry);
    }

private:
    //--------------------------------------------------------------------------
    // `EncIndBase`

    // we don't buffer reads or writes during benchmarked operations (search, update) except
    // during advances, keeping in line with the buffer's purpose of only compensating for
    // my slow implementation of advances during benchmarked operations

    bool SHOULD_BUFFER_READ(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncIndRand::SHOULD_BUFFER_READ(): wee mama zoo" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_WRITE(SseOper oper) const override {
        switch (oper) {
        case SseOper::SETUP:  return true;
        case SseOper::SEARCH: return false;
        case SseOper::UPDATE: return false;
        default:
            std::cerr << "Error: EncIndRand::SHOULD_BUFFER_WRITE(): wee mama zoo" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    bool SHOULD_BUFFER_ADVANCE(SseOper oper) const override { return true; }

    // this essentially means we have no buckets; each individual entry is a "bucket"
    bigint getBcktSize() const override { return 1; }
    bigint getBcktCount() const override { return this->capacity; }
};
