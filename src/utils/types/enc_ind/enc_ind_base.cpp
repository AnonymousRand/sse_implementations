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

    // init buffers
    bigint setupBufEntryCapacity = std::min(config::ENC_IND_SETUP_BUF_CAPACITY, this->capacity);
    this->setupBuf = new Buf(
        setupBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    bigint searchBufEntryCapacity = std::min(config::ENC_IND_SEARCH_BUF_CAPACITY, this->capacity);
    this->searchBuf = new Buf(
        searchBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );
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


bool EncIndBase::read(BufType bufType, ubigint pos, EncIndVal& ret, bool shouldFseek) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(bufType, pos, entry, shouldFseek);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndVal::fromUcstr(entry + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    // this method *should* only be called during searches
    BufType bufType = BufType::SEARCH;

    bool isFound = this->advanceUntilMatch(bufType, pos, key.c_str(), this->KEY_LEN());
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(bufType, pos, ret);
}


void EncIndBase::write(
    BufType bufType, ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek
) {
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
    this->writeEncoded(bufType, pos, encodedEntry.c_str());
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    // this method *should* only be called during setups
    BufType bufType = BufType::SETUP;

    bool isEmptyAvailable = this->advanceUntilMatch(
        bufType, pos, this->NULL_ENTRY, this->ENTRY_LEN()
    );
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
    this->write(bufType, pos, encIndEntry);
}


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry encIndEntry;
        // (`BufType::SETUP` here to just get a larger buffer; it shouldn't really matter here)
        this->readEntry(BufType::SETUP, pos, encIndEntry, pos == 0);
        std::cerr << pos << ": " << utils::debug::ustrToHex(encIndEntry.toUstr())
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


bool EncIndBase::advanceUntilMatch(
    BufType bufType, ubigint& pos, const uchar* match, int matchLen
) const {
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one position at a time to search for it
    bigint positionsChecked = 0;
    uchar currEntry[this->ENTRY_LEN()];
    this->readEncoded(bufType, pos, currEntry, true);
    while (std::memcmp(currEntry, match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->getBcktCount()) {
            return false;
        }

        pos = (pos + this->getBcktSize()) % this->capacity;
        if (this->getBcktSize() > 1 || pos < this->getBcktSize()) {
            // if either we need to `fseek()` to further than we had `fread()` (i.e.
            // `this->getBcktSize()` > 1), or `pos` had been decreased this iteration (i.e.
            // `pos < this->getBcktSize()`) meaning we must've wrapped around, call `fseek()`
            // to make sure we are on the correct position (otherwise the previous `fread()`
            // automatically handles it, so we can save some time)
            this->readEncoded(bufType, pos, currEntry, true);
        } else {
            this->readEncoded(bufType, pos, currEntry, false);
        }
    }

    return true;
}


void EncIndBase::readEncoded(BufType bufType, ubigint pos, uchar* ret, bool shouldFseek) const {
    Buf* bufToUse = this->getBufToUse(bufType);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos);
        bufIndex = 0;
    }
    assert(bufIndex < bufToUse->entryCapacity);

    utils::benchmark::startProfile("buf read");
    bufToUse->read(bufIndex, ret);
    utils::benchmark::stopProfile("buf read");
}


void EncIndBase::writeEncoded(
    BufType bufType, ubigint pos, const uchar* encodedEntry, bool shouldFseek
) {
    Buf* bufToUse = this->getBufToUse(bufType);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos);
        bufIndex = 0;
    }
    assert(bufIndex < bufToUse->entryCapacity);

    utils::benchmark::startProfile("buf write");
    bufToUse->write(bufIndex, encodedEntry);
    utils::benchmark::stopProfile("buf write");
}


bool EncIndBase::readEntry(BufType bufType, ubigint pos, EncIndEntry& ret, bool shouldFseek) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(bufType, pos, entry, shouldFseek);
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
    bigint entryCapacity,
    FILE* file, const std::string& filename, bigint encIndCapacity, bigint entryLen
) :
    entryCapacity(entryCapacity),
    file(file),
    filename(filename),
    encIndCapacity(encIndCapacity),
    entryLen(entryLen)
{
    this->data = new uchar[this->entryCapacity * this->entryLen];
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
    Buf(other.entryCapacity, other.file, other.filename, other.encIndCapacity, other.entryLen)
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


// TODO: is there a way to make this avoid a memcpy and return a pointer directly to
// this->data + (index * this->entryLen)? while still acommodating fseek of no-buffer approach?
// unless this memcpy isn't taking very much time
void EncIndBase::Buf::read(bigint index, uchar* ret) const {
    std::memcpy(ret, this->data + (index * this->entryLen), this->entryLen);
}


void EncIndBase::Buf::write(bigint index, const uchar* entry) {
    std::memcpy(this->data + (index * this->entryLen), entry, this->entryLen);
    this->isFlushed = false;
}


template <class SelfType> requires std::is_same_v<std::remove_cv_t<SelfType>, EncIndBase::Buf>
void EncIndBase::Buf::operOnFileBase(SelfType* self, OperType operType, ubigint startPos) {
    // this is the only place we check this
    if (self->entryCapacity <= 0) {
        return;
    }

    auto fileOper = [self, operType](uchar* data, bigint entryCount) {
        switch (operType) {
        // curly braces used to prevent "crosses initialization of" error with `itemsRead`
        case OperType::FILL: {
            utils::benchmark::startProfile("fread");
            bigint itemsRead = std::fread(data, self->entryLen, entryCount, self->file);
            utils::benchmark::stopProfile("fread");
            return itemsRead;
        }
        case OperType::FLUSH: {
            utils::benchmark::startProfile("fwrite");
            bigint itemsWritten = std::fwrite(data, self->entryLen, entryCount, self->file);
            utils::benchmark::stopProfile("fwrite");
            return itemsWritten;
        }
        default:
            std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): wee zoo mama" << std::endl;
            std::exit(EXIT_FAILURE);
            break;
        }
    };

    // first operate on as many of the target entries as we can without exceeding EOF
    bigint entriesUntilEof = self->encIndCapacity - startPos;
    bigint entriesToOper1 = std::min(self->entryCapacity, entriesUntilEof);
    utils::benchmark::startProfile("fseek");
    std::fseek(self->file, startPos * self->entryLen, SEEK_SET);
    utils::benchmark::stopProfile("fseek");
    bigint itemsOpered = fileOper(self->data, entriesToOper1);
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
    if (entriesToOper1 < self->entryCapacity) {
        utils::benchmark::startProfile("fseek");
        std::fseek(self->file, 0, SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        itemsOpered += fileOper(
            self->data + (entriesToOper1 * self->entryLen), self->entryCapacity - entriesToOper1
        );
        DEBUG_ONLY({
            if (itemsOpered < self->entryCapacity) {
                std::cerr << "Error: EncIndBase::Buf::operOnFileBase(): error operating (part 2) "
                          << "on file " << self->filename
                          << " (only did " << itemsOpered << " out of " << self->entryCapacity
                          << ")" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
}


void EncIndBase::Buf::fill(ubigint startPos) {
    operOnFileBase(this, OperType::FILL, startPos);
    this->startPos = startPos;
    this->endPos = (startPos + this->entryCapacity) % this->encIndCapacity;
    this->isFilled = true;
    this->isFlushed = true;
}


void EncIndBase::Buf::flushIfNotFlushed() const {
    if (!this->isFlushed && this->isFilled) {
        operOnFileBase(this, OperType::FLUSH, this->startPos);
        this->isFlushed = true;
    }
}


bigint EncIndBase::Buf::posToBufIndex(ubigint pos) const {
    if (!this->isFilled || this->entryCapacity == 0) {
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


void EncIndBase::fillBuf(Buf* buf, ubigint bufStartPos) const {
    this->flushBufIfNotFlushed(buf);
    this->flushIfNotFlushed();
    buf->fill(bufStartPos);
}


void EncIndBase::flushBufIfNotFlushed(Buf* buf) const {
    buf->flushIfNotFlushed();
    this->isFlushed = false;
}


bigint EncIndBase::posToBufIndex(Buf* buf, ubigint pos) const {
    return buf->posToBufIndex(pos);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void EncIndBase::Buf::operOnFileBase(Buf* self, OperType operType, ubigint startPos);
template void EncIndBase::Buf::operOnFileBase(const Buf* self, OperType operType, ubigint startPos);
