#include "utils/types/encr_ind/encr_ind_base_buf.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <string>

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base.h"
#include "utils/types/ustring.h"


const bigint EncrIndBase::Buf::NOT_IN_BUF = -1;


//------------------------------------------------------------------------------
// constructors/destructors


EncrIndBase::Buf::Buf(
    bigint ENTRY_CAPACITY,
    FILE* file, const std::string& filename, bigint encrIndCapacity, bigint entryLen
) :
    ENTRY_CAPACITY(ENTRY_CAPACITY),
    file(file),
    filename(filename),
    encrIndCapacity(encrIndCapacity),
    entryLen(entryLen)
{
    this->data = new uchar[this->ENTRY_CAPACITY * this->entryLen];
    assert(this->ENTRY_CAPACITY <= this->encrIndCapacity);
}


EncrIndBase::Buf::~Buf() {
    if (this->data != nullptr) {
        delete[] this->data;
        this->data = nullptr;
    }
}


//------------------------------------------------------------------------------
// rule of five


EncrIndBase::Buf::Buf(const Buf& other) :
    Buf(other.ENTRY_CAPACITY, other.file, other.filename, other.encrIndCapacity, other.entryLen)
{
    if (other.data != nullptr) {
        this->data = new uchar[](*other.data);
    } else {
        this->data = nullptr;
    }

    this->startPos = other.startPos;
    this->endPos = other.endPos;
    this->isFilled = other.isFilled;
    this->isFlushed = other.isFlushed;
}


//------------------------------------------------------------------------------
// interface


uchar* EncrIndBase::Buf::read(bigint index) const {
    assert(index < this->ENTRY_CAPACITY || this->ENTRY_CAPACITY == 0);
    return this->data + (index * this->entryLen);
}


void EncrIndBase::Buf::write(bigint index, const uchar* entry) {
    assert(index < this->ENTRY_CAPACITY || this->ENTRY_CAPACITY == 0);
    std::memcpy(this->data + (index * this->entryLen), entry, this->entryLen);
    this->isFlushed = false;
}


template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, EncrIndBase::Buf>
void EncrIndBase::Buf::operOnFileBase(
    SelfType* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
) {
    assert(startPos < self->encrIndCapacity);
    // we need to allow this, e.g. for when SSE schemes are set up with an empty DB
    if (self->ENTRY_CAPACITY <= 0) {
        return;
    }

    // first operate on as many of the target entries as we can without exceeding EOF
    bigint entriesUntilEof = self->encrIndCapacity - startPos;
    bigint entriesToOper1 = std::min(self->ENTRY_CAPACITY, entriesUntilEof);
    // we always `fseek()` here since we were likely reading from the buffer previously,
    // and that doesn't advance the file pointers
    std::fseek(self->file, startPos * self->entryLen, SEEK_SET);
    bigint itemsOpered = oper(self->data, entriesToOper1);
    DEBUG_ONLY({
        if (itemsOpered < entriesToOper1) {
            std::perror(std::format(
                "Error: EncrIndBase::Buf::operOnFileBase(): error operating (part 1) on file {} "
                    "(only did {} out of {})",
                self->filename, itemsOpered, entriesToOper1
            ).c_str());
            std::exit(EXIT_FAILURE);
        }
    });

    // wrap around to beginning of file if we read less than the target number of entries
    // (NOTE: the buf must not be larger than `this->encrIndCapacity`, so that we only need to
    // wrap around at most once!)
    if (entriesToOper1 < self->ENTRY_CAPACITY) {
        std::fseek(self->file, 0, SEEK_SET);
        itemsOpered += oper(
            self->data + (entriesToOper1 * self->entryLen),
            self->ENTRY_CAPACITY - entriesToOper1
        );
        DEBUG_ONLY({
            if (itemsOpered < self->ENTRY_CAPACITY) {
                std::perror(std::format(
                    "Error: EncrIndBase::Buf::operOnFileBase(): error operating (part 2) on file {}"
                        " (only did {} across both parts out of {})",
                    self->filename, itemsOpered, self->ENTRY_CAPACITY
                ).c_str());
                std::exit(EXIT_FAILURE);
            }
        });
    }
}


// IMPORTANT: this doesn't control flushing of the `FILE*`, so that's the encr ind's responsibility!
void EncrIndBase::Buf::fill(ubigint startPos, bool allowIncompleteFill) {
    auto fillOper = [this, allowIncompleteFill](uchar* data, bigint targetEntryCount) {
        bigint itemsRead = std::fread(data, this->entryLen, targetEntryCount, this->file);
        if (allowIncompleteFill) {
            return targetEntryCount;
        } else {
            return itemsRead;
        }
    };
    operOnFileBase(this, fillOper, startPos);

    this->startPos = startPos;
    this->endPos = (startPos + this->ENTRY_CAPACITY) % this->encrIndCapacity;
    this->isFilled = true;
    this->isFlushed = true;
}


// IMPORTANT: this doesn't control flushing of the `FILE*`, so that's the encr ind's responsibility!
void EncrIndBase::Buf::flushIfNotFlushed() const {
    if (!this->isFlushed && this->isFilled) {
        auto flushOper = [this](uchar* data, bigint targetEntryCount) {
            return std::fwrite(data, this->entryLen, targetEntryCount, this->file);
        };
        operOnFileBase(this, flushOper, this->startPos);
        this->isFlushed = true;
    }
}


bigint EncrIndBase::Buf::posToBufIndex(ubigint pos) const {
    if (!this->isFilled || this->ENTRY_CAPACITY == 0) {
        return NOT_IN_BUF;
    }

    if (this->startPos < this->endPos) {
        if (pos >= this->startPos && pos < this->endPos) {
            // if `this` did not reach or wrap around the end of the file
            // and `pos` is in the middle of it (note that this also means the following
            // returned value should always be positive)
            bigint ret = pos - this->startPos;
            assert(ret >= 0 && ret < this->ENTRY_CAPACITY);
            return ret;
        }
    } else {
        // if `this` does reach or wrap around the end of the file
        if (pos >= this->startPos) {
            // if `pos` is at/after the buffer's start pos (so `pos` hasn't wrapped around yet)
            bigint ret = pos - this->startPos;
            assert(ret >= 0 && ret < this->ENTRY_CAPACITY);
            return ret;
        } else if (pos < this->endPos) {
            // if `pos` is before the buffer's end pos (so `pos` did wrap around)

            // we add up the segment from `pos` to the start of the encr ind,
            // and the segment from the end of the encr ind to `this->startPos`
            bigint ret = pos + (this->encrIndCapacity - this->startPos);
            assert(ret >= 0 && ret < this->ENTRY_CAPACITY);
            return ret;
        }
    }

    return NOT_IN_BUF;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void EncrIndBase::Buf::operOnFileBase(
    Buf* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
);
template void EncrIndBase::Buf::operOnFileBase(
    const Buf* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
);
