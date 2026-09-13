#include "utils/types/enc_ind/enc_ind_base.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// constructors/destructors


EncIndBase::~EncIndBase() {
    this->clear();
}


//------------------------------------------------------------------------------
// rule of five


void EncIndBase::copyFrom(const EncIndBase& other) {
    IDiskStorage::copyFrom(other);

    if (other.NULL_ENTRY != nullptr) {
        assert(this->ENTRY_LEN() == other.ENTRY_LEN());
        this->NULL_ENTRY = new uchar[this->ENTRY_LEN()];
        std::memcpy(this->NULL_ENTRY, other.NULL_ENTRY, this->ENTRY_LEN());
    } else {
        this->NULL_ENTRY = nullptr;
    }
    this->capacity = other.capacity;

    if (other.buf != nullptr) {
        // this triggers `Buf`'s copy constructor
        this->buf = new Buf(*other.buf);
    } else {
        this->buf = nullptr;
    }
    this->isBufFlushed = other.isBufFlushed;
}


void EncIndBase::moveFrom(EncIndBase&& other) noexcept {
    IDiskStorage::moveFrom(other);

    // this is now regular pointer assignment instead of actually copying the heap data
    this->NULL_ENTRY = other.NULL_ENTRY;
    other.NULL_ENTRY = nullptr;

    this->capacity = other.capacity;

    this->buf = other.buf;
    other.buf = nullptr;

    this->isBufFlushed = other.isBufFlushed;
}


// copy constructor
EncIndBase::EncIndBase(const EncIndBase& other) {
    this->copyFrom(other);
}


// copy assignment operator
EncIndBase& EncIndBase::operator =(const EncIndBase& other) {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->copyFrom(other);
    }
    return *this;
}


// move constructor
EncIndBase::EncIndBase(EncIndBase&& other) {
    this->moveFrom(std::move(other));
}


// move assignment operator
EncIndBase& EncIndBase::operator =(EncIndBase&& other) {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->moveFrom(std::move(other));
    }
    return *this;
}


//------------------------------------------------------------------------------
// interface


void EncIndBase::init(bigint capacity) {
    // inits enc ind file and file pointer
    IDiskStorage::init();

    // fill file with zero bits (so we can tell if a spot is empty by if it contains all zero bits)

    // also initialize `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()()` relies on virtual methods
    // (technically it is possible that an encrypted tuple happens to be all '0' bytes and thus gets
    // mistaken for a null kv pair, but currently `this->ENTRY_LEN()` is >1000 bits so there's
    // a 2^{>1000} chance of this happening...and USENIX'24's implementation just does this too)
    this->NULL_ENTRY = new uchar[this->ENTRY_LEN()] {};
    this->capacity = capacity;
    utils::benchmark::startProfile("init");
    for (bigint i = 0; i < this->capacity; i++) {
        int itemsWritten = std::fwrite(this->NULL_ENTRY, this->ENTRY_LEN(), 1, this->file);
        DEBUG_ONLY({
            if (itemsWritten != 1) {
                std::cerr << "Error: EncIndBase::init(): error initializing file " << this->filename
                          << " with zero bits (nothing written)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
    std::fflush(this->file);
    utils::benchmark::stopProfile("init");

    // init buffer
    bigint bufEntryCapacity = std::min(config::ENC_IND_READ_BUF_CAPACITY, this->capacity);
    this->buf = new Buf(bufEntryCapacity, this->ENTRY_LEN());
}


void EncIndBase::clear() {
    if (this->NULL_ENTRY != nullptr) {
        delete[] this->NULL_ENTRY;
        this->NULL_ENTRY = nullptr;
    }
    this->capacity = 0;

    if (this->buf != nullptr) {
        delete this->buf;
        this->buf = nullptr;
    }

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(ubigint pos, EncIndVal& ret, bool shouldFseek) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(pos, entry, shouldFseek);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndVal::fromUcstr(entry + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(pos, key.c_str(), this->KEY_LEN());
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek) {
    pos %= this->capacity;

    // encode `encIndEntry` into one string
    ustring encodedEntry = encIndEntry.toUstr();
    DEBUG_ONLY({
        if (encodedEntry.length() != this->ENTRY_LEN()) {
            std::cerr << "Error: EncIndBase::write(): write of length " << encodedEntry.length()
                      << " bytes is not allowed! (want " << this->ENTRY_LEN() << " bytes)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // then go to `pos` and write the encoded `encIndEntry`
    if (shouldFseek) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
    }
    this->writeEncoded(pos, encodedEntry.c_str());
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(pos, this->NULL_ENTRY, this->ENTRY_LEN());
    // if we've scoured the whole index and still haven't found an available space,
    // throw an error: we are trying to write to a full index
    DEBUG_ONLY({
        if (!isEmptyAvailable) {
            std::cerr << "Error: EncIndBase::writeToFirstEmpty(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // write into the empty location we found
    this->write(pos, encIndEntry);
}


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry encIndEntry;
        this->readEntry(pos, encIndEntry, pos == 0);
        std::cerr << pos << ": " << utils::debug::ustrToHex(encIndEntry.toUstr())
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


void EncIndBase::readEncoded(ubigint pos, uchar* ret, bool shouldFseek) const {
    // if no buffer requested
    if (this->buf->ENTRY_CAPACITY == 0) {
        this->flushIfNotFlushed();

        if (shouldFseek) {
            utils::benchmark::startProfile("fseek");
            std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
            utils::benchmark::stopProfile("fseek");
        }
        utils::benchmark::startProfile("fread");
        bigint itemsRead = std::fread(ret, this->ENTRY_LEN(), 1, this->file);
        utils::benchmark::stopProfile("fread");
        DEBUG_ONLY({
            if (itemsRead != 1) {
                std::cerr << "Error: EncIndBase::readEncoded(): error reading from file "
                          << this->filename << " (nothing read)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });

        return;
    }

    bigint bufIndex = this->posToBufIndex(pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(pos);
        bufIndex = 0;
    }

    utils::benchmark::startProfile("buf read");
    ret = this->buf->read(bufIndex);
    utils::benchmark::stopProfile("buf read");
}


void EncIndBase::writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek) {
    // if no buffer requested
    if (this->buf->ENTRY_CAPACITY == 0) {
        if (shouldFseek) {
            utils::benchmark::startProfile("fseek");
            std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
            utils::benchmark::stopProfile("fseek");
        }
        utils::benchmark::startProfile("fwrite");
        int itemsWritten = std::fwrite(encodedEntry, this->ENTRY_LEN(), 1, this->file);
        utils::benchmark::stopProfile("fwrite");
        DEBUG_ONLY({
            if (itemsWritten != 1) {
                std::cerr << "Error: EncIndBase::writeEncoded(): error writing to file "
                          << this->filename << " (nothing written)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });

        this->isFlushed = false;
        return;
    }

    bigint bufIndex = this->posToBufIndex(pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(pos);
        bufIndex = 0;
    }

    utils::benchmark::startProfile("buf write");
    this->buf->write(bufIndex, encodedEntry);
    utils::benchmark::stopProfile("buf write");
    this->isBufFlushed = false;
}


bool EncIndBase::readEntry(ubigint pos, EncIndEntry& ret, bool shouldFseek) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(pos, entry, shouldFseek);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndEntry::fromUcstr(entry, this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


//==============================================================================
// `EncIndBase::Buf`
//==============================================================================


const bigint EncIndBase::Buf::NOT_IN_BUF = -1;


EncIndBase::Buf::Buf(bigint ENTRY_CAPACITY, bigint ENTRY_LEN) :
    ENTRY_CAPACITY(ENTRY_CAPACITY),
    ENTRY_LEN(ENTRY_LEN)
{
    this->data = new uchar[this->ENTRY_CAPACITY * this->ENTRY_LEN];
}


EncIndBase::Buf::~Buf() {
    if (this->data != nullptr) {
        delete[] this->data;
        this->data = nullptr;
    }
}


const uchar* EncIndBase::Buf::read(bigint index) const {
    return this->data + (index * this->ENTRY_LEN);
}


void EncIndBase::Buf::write(bigint index, const uchar* entry) {
    std::memcpy(this->data + (index * this->ENTRY_LEN), entry, this->ENTRY_LEN);
}


template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, Buf>
bigint EncIndBase::operOnFileBase(
    SelfType* self,
    FILE* file, const std::string& filename, bool isRead,
    bigint entriesToOper, bigint encIndCapacity
) {
    if (entriesToOper <= 0) {
        return;
    }

    auto fileOper = [self, file, isRead](uchar* data, bigint entryCount) {
        if (isRead) {
            utils::benchmark::startProfile("fread");
            bigint itemsRead = std::fread(data, self->ENTRY_LEN, entryCount, file);
            utils::benchmark::stopProfile("fread");
            return itemsRead;
        } else {
            utils::benchmark::startProfile("fwrite");
            bigint itemsWritten = std::fwrite(data, self->ENTRY_LEN, entryCount, file);
            utils::benchmark::stopProfile("fwrite");
            return itemsWritten;
        }
    };

    // first operate on as many of the target entries as we can without exceeding EOF
    bigint entriesUntilEof = encIndCapacity - self->startPos;
    bigint entriesToOper1 = std::min(entriesToOper, entriesUntilEof);
    utils::benchmark::startProfile("fseek");
    std::fseek(file, self->startPos * self->ENTRY_LEN, SEEK_SET);
    utils::benchmark::stopProfile("fseek");
    bigint itemsOpered = fileOper(self->data, entriesToOper1);
    DEBUG_ONLY({
        if (itemsRead < entriesToOper1) {
            std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): error operating (part 1) "
                      << "on file " << filename
                      << " (only did " << itemsOpered << " out of " << entriesToOper1 << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // wrap around to beginning of file if we read less than the target number of entries
    // (NOTE: the buf must not be larger than `encIndCapacity`, so that we only need to
    // wrap around at most once!)
    if (entriesToOper1 < entriesToOper) {
        utils::benchmark::startProfile("fseek");
        std::fseek(file, 0, SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        itemsOpered += fileOper(
            self->data + (entriesToOper1 * self->ENTRY_LEN), entriesToOper - entriesToOper1
        );
        DEBUG_ONLY({
            if (itemsOpered < entriesToOper) {
                std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): error operating (part 2) "
                          << "on file " << self->filename
                          << " (only did " << itemsOpered << " out of " << entriesToOper << ")"
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
}


template void EncIndBase::Buf::operOnFileBase(
    Buf* self,
    FILE* file, const std::string& filename, bool isRead,
    bigint entriesToOper, bigint encIndCapacity
);
template void EncIndBase::Buf::operOnFileBase(
    const Buf* self,
    FILE* file, const std::string& filename, bool isRead,
    bigint entriesToOper, bigint encIndCapacity
);


void EncIndBase::Buf::fill(
    FILE* file, const std::string& filename, ubigint startPos, bigint encIndCapacity
) {
    operOnFileBase(this, file, filename, true, this->ENTRY_CAPACITY, encIndCapacity);
    this->startPos = startPos;
    this->endPos = (startPos + this->ENTRY_CAPACITY) % encIndCapacity;
    //std::cout << "current end pos is now " << this->currBufEndPos << " while actual current end pos is " << (float)std::ftell(this->file) / this->ENTRY_LEN() << std::endl;
}


void EncIndBase::Buf::flush(FILE* file, const std::string& filename, bigint encIndCapacity) const {
    operOnFileBase(this, file, filename, false, this->ENTRY_CAPACITY, encIndCapacity);
}


bigint EncIndBase::Buf::posToBufIndex(ubigint pos, bigint encIndCapacity) const {
    if (this->ENTRY_CAPACITY == 0) {
        return NOT_IN_BUF;
    }

    if (this->startPos < this->endPos) {
        if (pos >= this->startPos && pos < this->endPos) {
            // if `this` did not reach or wrap around the end of the file
            // and `pos` is in the middle of it (note that this also means the following
            // returned value should always be positive)
            bigint ret = pos - this->startPos;
            assert(ret >= 0);
            return ret;
        }
    } else {
        // if `this` does reach or wrap around the end of the file
        if (pos >= this->startPos) {
            // if `pos` is at/after the buffer's start pos (so `pos` hasn't wrapped around yet)
            bigint ret = pos - this->startPos;
            assert(ret >= 0);
            return ret;
        } else if (pos < this->endPos) {
            // if `pos` is before the buffer's end pos (so `pos` did wrap around)

            // we add up the segment from `pos` to the start of the enc ind,
            // and the segment from the end of the enc ind to `this->startPos`
            bigint ret = pos + (capacity - this->startPos);
            assert(ret >= 0);
            return ret;
        }
    }
    return NOT_IN_BUF;
}


//------------------------------------------------------------------------------
// helpers


void EncIndBase::fillBuf(ubigint bufStartPos) const {
    this->flushBufIfNotFlushed();
    this->flushIfNotFlushed();
    this->buf->fill(this->file, this->filename, bufStartPos, this->capacity);
}


void EncIndBase::flushBufIfNotFlushed() const {
    if (!this->isBufFlushed) {
        this->buf->flush(this->file, this->filename);
        this->isBufFlushed = true;
        this->isFlushed = false;
    }
}


bigint EncIndBase::posToBufIndex(ubigint pos) const {
    return this->buf->posToBufIndex(pos, this->capacity);
}
