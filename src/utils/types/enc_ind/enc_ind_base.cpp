#include "utils/types/enc_ind/enc_ind_base.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

    if (other.setupBuf != nullptr) {
        // this triggers `Buf`'s copy constructor
        this->setupBuf = new Buf(*other.setupBuf);
    } else {
        this->setupBuf = nullptr;
    }

    if (other.searchBuf != nullptr) {
        this->searchBuf = new Buf(*other.searchBuf);
    } else {
        this->searchBuf = nullptr;
    }
}


void EncIndBase::moveFrom(EncIndBase&& other) noexcept {
    IDiskStorage::moveFrom(std::move(other));

    // this is now regular pointer assignment instead of actually copying the heap data
    this->NULL_ENTRY = other.NULL_ENTRY;
    other.NULL_ENTRY = nullptr;

    this->capacity = other.capacity;

    this->setupBuf = other.setupBuf;
    other.setupBuf = nullptr;

    this->searchBuf = other.searchBuf;
    other.searchBuf = nullptr;
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
EncIndBase::EncIndBase(EncIndBase&& other) noexcept {
    this->moveFrom(std::move(other));
}


// move assignment operator
EncIndBase& EncIndBase::operator =(EncIndBase&& other) noexcept {
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

    // also initialize `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()()` relies on virtual methods
    // (technically it is possible that an encrypted tuple happens to be all '0' bytes and thus gets
    // mistaken for a null kv pair, but currently `this->ENTRY_LEN()` is >1000 bits so there's
    // a 2^{>1000} chance of this happening...and USENIX'24's implementation just does this too)
    this->NULL_ENTRY = new uchar[this->ENTRY_LEN()] {};
    this->capacity = capacity;

    // init buffers
    bigint setupBufEntryCapacity = std::min(config::ENC_IND_SETUP_BUF_CAPACITY, this->capacity);
    this->setupBuf = new Buf(
        setupBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    bigint searchBufEntryCapacity = std::min(config::ENC_IND_SEARCH_BUF_CAPACITY, this->capacity);
    this->searchBuf = new Buf(
        searchBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    // fill file with zero bits, so we can tell if a spot is empty by if it contains all zero bits
    // and use setup buffer to speed this up (although this seems to only be efficient at big sizes)
    utils::benchmark::startProfile("init");
    for (bigint i = 0; i < this->capacity; i++) {
        this->writeEncoded(Oper::SETUP, i, this->NULL_ENTRY, true);
    }
    this->flushBufIfNotFlushed(this->setupBuf);
    utils::benchmark::stopProfile("init");
}


void EncIndBase::clear() {
    if (this->NULL_ENTRY != nullptr) {
        delete[] this->NULL_ENTRY;
        this->NULL_ENTRY = nullptr;
    }
    this->capacity = 0;

    if (this->setupBuf != nullptr) {
        delete this->setupBuf;
        this->setupBuf = nullptr;
    }
    if (this->searchBuf != nullptr) {
        delete this->searchBuf;
        this->searchBuf = nullptr;
    }

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(Oper oper, ubigint pos, EncIndVal& ret) const {
    // read encoded entry at `pos`
    uchar* entry = this->readEncoded(oper, pos);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    // decode entry
    ret = EncIndVal::fromUcstr(entry + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(Oper oper, ubigint& pos, const ustring& key, EncIndVal& ret) const {
    std::cout << "+++++ finding " << std::endl;
    bool isFound = this->advanceUntilMatch(oper, pos, key.c_str(), this->KEY_LEN());
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(oper, pos, ret);
}


void EncIndBase::write(Oper oper, ubigint pos, const EncIndEntry& encIndEntry) {
    // encode `encIndEntry`
    ustring encodedEntry = encIndEntry.toUstr();
    DEBUG_ONLY({
        if (encodedEntry.length() != this->ENTRY_LEN()) {
            std::cerr << "Error: EncIndBase::write(): write of length " << encodedEntry.length()
                      << " bytes is not allowed! (want " << this->ENTRY_LEN() << " bytes)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // write encoded entry to `pos`
    this->writeEncoded(oper, pos, encodedEntry.c_str());
}


void EncIndBase::writeToFirstEmpty(Oper oper, ubigint& pos, const EncIndEntry& encIndEntry) {
    std::cout << "----- writing " << std::endl;
    bool isEmptyAvailable = this->advanceUntilMatch(oper, pos, this->NULL_ENTRY, this->ENTRY_LEN());
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
    this->write(oper, pos, encIndEntry);
}


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry encIndEntry;
        // (`Oper::SETUP` here to just get a larger buffer; it shouldn't really matter here)
        this->readEntry(Oper::SETUP, pos, encIndEntry);
        std::cerr << pos << ": " << utils::debug::ustrToHex(encIndEntry.toUstr())
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


bool EncIndBase::advanceUntilMatch(
    Oper oper, ubigint& pos, const uchar* match, int matchLen
) const {
    // need this for wrapping logic later to work!
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one bucket (i.e. `this->getBcktSize()`) at a time to search for it

    // for the first read, we read directly from the file, so that if it turns out we don't need to
    // iterate forward, we skip filling the buffer. this is especially good when buffer is big but
    // enc ind is even bigger, as this avoids large amounts of filling and flushing the buffer at
    // different positions and never using it in between when the enc ind is still mostly empty
    uchar currEntry[this->ENTRY_LEN()];
    // >>TODO OHHHHH readEncodedOptionalBuf is not good because it misses previous non-flushed writes!!
    // so maybe do a read method that doesn't fill up the buffer if pos is NOT_IN_BUF, rather reads
    // from the file instead
    this->readEncodedOptionalBuf(oper, pos, currEntry, true);
    if (std::memcmp(currEntry, match, matchLen) == 0) {
        std::cout << "success, pos is " << pos << " and currEntry is " << utils::debug::ustrToHex(currEntry, 16) << std::endl;
        return true;
    }
    std::cout << "not first success" << std::endl;

    // if we do need to iterate forward, then fill the buffer if needed and read from it
    // importantly, if we are skipping entries (i.e. `this->getBcktSize() > 1`), then we don't
    // buffer searches as we aren't gonna read most of the buffer anyway, so we get to save filling
    // and flushing it constantly (and searches usually don't need us to iterate forward huge
    // amounts unlike the end of setup phases, so filling such large buffers is especially wasteful)
    uchar* currEntryPtr;
    bigint positionsChecked = 0;
    if (this->getBcktSize() != 1) {
        std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA " << this->getBcktSize() << std::endl << std::endl << std::endl;
    }
    do {
        std::cout << "checking: addr " << (void*)currEntryPtr << " and value " << utils::debug::ustrToHex(currEntryPtr, 16) << " and match is " << utils::debug::ustrToHex(match, 16) << std::endl;
        positionsChecked++;
        if (positionsChecked == this->getBcktCount()) {
            return false;
        }

        pos = (pos + this->getBcktSize()) % this->capacity;
        if (oper == Oper::SETUP || this->getBcktSize() == 1) {
            currEntryPtr = this->readEncoded(oper, pos);
        } else {
            // also, we don't `fseek()` for this read unless we have wrapped around to the beginning
            // of the file via `pos = ... % this->capacity` or if we are skipping entries (i.e.
            // `this->getBcktSize() > 1`; yes i know this is technically always true here), as
            // otherwise the previous `fread()` should've moved the file pointer to the right pos
            bool shouldFseek = this->getBcktSize() > 1 || pos < this->getBcktSize();
            this->readEncodedOptionalBuf(oper, pos, currEntry, shouldFseek);
            currEntryPtr = currEntry;
        }
    } while (std::memcmp(currEntryPtr, match, matchLen) != 0);

    std::cout << "eventual success " << std::endl;
    return true;
}


uchar* EncIndBase::readEncoded(Oper oper, ubigint pos) const {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos);
        bufIndex = 0;
    }
    assert(bufIndex < bufToUse->ENTRY_CAPACITY);

    utils::benchmark::startProfile("buf read");
    uchar* ret = bufToUse->read(bufIndex);
    utils::benchmark::stopProfile("buf read");
    return ret;
}


void EncIndBase::writeEncoded(
    Oper oper, ubigint pos, const uchar* encodedEntry, bool isInit
) {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos, isInit);
        bufIndex = 0;
    }
    assert(bufIndex < bufToUse->ENTRY_CAPACITY);

    utils::benchmark::startProfile("buf write");
    bufToUse->write(bufIndex, encodedEntry);
    utils::benchmark::stopProfile("buf write");
}


void EncIndBase::readEncodedOptionalBuf(
    Oper oper, ubigint pos, uchar* ret, bool shouldFseek
) const {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        // if `pos` is not covered by buffer, fetch directly from file; we can completely ignore
        // the buffer here as the file must have the most updated copy of the entry at `pos`
        if (shouldFseek) {
            utils::benchmark::startProfile("fseek");
            std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
            utils::benchmark::stopProfile("fseek");
        }
        utils::benchmark::startProfile("fread");
        int itemsRead = std::fread(ret, this->ENTRY_LEN(), 1, this->file);
        utils::benchmark::stopProfile("fread");
        DEBUG_ONLY({
            if (itemsRead != 1) {
                std::cerr << "Error: EncIndBase::readEncodedOptionalBuf(): error reading from file "
                          << this->filename << " (nothing read)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    } else {
        // if `pos` is covered by the buffer, read it from the buffer instead since the buffer may
        // have a more updated version of that entry than the file
        // the way we pass `ret` to accommodate the `fread()` approach above forces `memcpy()` here
        utils::benchmark::startProfile("buf read");
        std::memcpy(ret, bufToUse->read(bufIndex), this->ENTRY_LEN());
        utils::benchmark::stopProfile("buf read");
    }
}


bool EncIndBase::readEntry(Oper oper, ubigint pos, EncIndEntry& ret) const {
    uchar* entry = this->readEncoded(oper, pos);
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


//------------------------------------------------------------------------------
// constructors/destructors


EncIndBase::Buf::Buf(
    bigint ENTRY_CAPACITY,
    FILE* file, const std::string& filename, bigint encIndCapacity, bigint entryLen
) :
    ENTRY_CAPACITY(ENTRY_CAPACITY),
    file(file),
    filename(filename),
    encIndCapacity(encIndCapacity),
    entryLen(entryLen)
{
    this->data = new uchar[this->ENTRY_CAPACITY * this->entryLen];
}


EncIndBase::Buf::~Buf() {
    if (this->data != nullptr) {
        delete[] this->data;
        this->data = nullptr;
    }
}


//------------------------------------------------------------------------------
// rule of five


EncIndBase::Buf::Buf(const Buf& other) :
    Buf(other.ENTRY_CAPACITY, other.file, other.filename, other.encIndCapacity, other.entryLen)
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


uchar* EncIndBase::Buf::read(bigint index) const {
    return this->data + (index * this->entryLen);
}


void EncIndBase::Buf::write(bigint index, const uchar* entry) {
    std::memcpy(this->data + (index * this->entryLen), entry, this->entryLen);
    this->isFlushed = false;
}


template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, EncIndBase::Buf>
void EncIndBase::Buf::operOnFileBase(
    SelfType* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
) {
    // this is the only place we check this
    assert(self->ENTRY_CAPACITY > 0);

    // first operate on as many of the target entries as we can without exceeding EOF
    bigint entriesUntilEof = self->encIndCapacity - startPos;
    bigint entriesToOper1 = std::min(self->ENTRY_CAPACITY, entriesUntilEof);
    // we always `fseek()` here since we were likely reading from the buffer previously,
    // and that doesn't advance the file pointers
    std::fseek(self->file, startPos * self->entryLen, SEEK_SET);
    bigint itemsOpered = oper(self->data, entriesToOper1);
    DEBUG_ONLY({
        if (itemsOpered < entriesToOper1) {
            std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): error operating (part 1) "
                      << "on file " << self->filename
                      << " (only did " << itemsOpered << " out of " << entriesToOper1 << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // wrap around to beginning of file if we read less than the target number of entries
    // (NOTE: the buf must not be larger than `this->encIndCapacity`, so that we only need to
    // wrap around at most once!)
    if (entriesToOper1 < self->ENTRY_CAPACITY) {
        std::fseek(self->file, 0, SEEK_SET);
        itemsOpered += oper(
            self->data + (entriesToOper1 * self->entryLen),
            self->ENTRY_CAPACITY - entriesToOper1
        );
        DEBUG_ONLY({
            if (itemsOpered < self->ENTRY_CAPACITY) {
                std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): error operating (part 2) "
                          << "on file " << self->filename
                          << " (only did " << itemsOpered << " out of " << self->ENTRY_CAPACITY
                          << ")" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
}


void EncIndBase::Buf::fill(ubigint startPos, bool allowIncompleteFill) {
    auto fillOper = [this, allowIncompleteFill](uchar* data, bigint targetEntryCount) {
        utils::benchmark::startProfile("buf fill");
        bigint itemsRead = std::fread(data, this->entryLen, targetEntryCount, this->file);
        utils::benchmark::stopProfile("buf fill");
        if (allowIncompleteFill) {
            return targetEntryCount;
        } else {
            return itemsRead;
        }
    };
    operOnFileBase(this, fillOper, startPos);

    this->startPos = startPos;
    this->endPos = (startPos + this->ENTRY_CAPACITY) % this->encIndCapacity;
    this->isFilled = true;
    this->isFlushed = true;
}


void EncIndBase::Buf::flushIfNotFlushed() const {
    if (!this->isFlushed && this->isFilled) {
        auto flushOper = [this](uchar* data, bigint targetEntryCount) {
            utils::benchmark::startProfile("buf flush");
            bigint itemsWritten = std::fwrite(data, this->entryLen, targetEntryCount, this->file);
            utils::benchmark::stopProfile("buf flush");
            return itemsWritten;
        };
        operOnFileBase(this, flushOper, this->startPos);

        this->isFlushed = true;
    }
}


bigint EncIndBase::Buf::posToBufIndex(ubigint pos) const {
    if (!this->isFilled || this->ENTRY_CAPACITY == 0) {
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
            bigint ret = pos + (this->encIndCapacity - this->startPos);
            assert(ret >= 0);
            return ret;
        }
    }

    return NOT_IN_BUF;
}


//------------------------------------------------------------------------------
// `EncIndBase` helpers


void EncIndBase::fillBuf(Buf* buf, ubigint bufStartPos, bool isEncIndInit) const {
    this->flushBufIfNotFlushed(buf);
    buf->fill(bufStartPos, isEncIndInit);
}


void EncIndBase::flushBufIfNotFlushed(Buf* buf) const {
    buf->flushIfNotFlushed();
    this->isFlushed = false;
    // remember to then flush the fwrite buffer to the file too
    this->flushIfNotFlushed();
}


bigint EncIndBase::posToBufIndex(Buf* buf, ubigint pos) const {
    return buf->posToBufIndex(pos);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void EncIndBase::Buf::operOnFileBase(
    Buf* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
);
template void EncIndBase::Buf::operOnFileBase(
    const Buf* self, const std::function<bigint(uchar*, bigint)>& oper, ubigint startPos
);
