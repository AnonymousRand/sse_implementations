#include "utils/types/enc_ind/enc_ind_base.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base_buf.h"
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
        // we allow incomplete buffer fills from the file here since, well, the file is incomplete
        this->writeEncoded(Oper::SETUP, i, this->NULL_ENTRY, true);
    }
    this->flushBufIfNotFlushed(this->getBufFromOper(Oper::SETUP));
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


bool EncIndBase::read(Oper oper, ubigint pos, EncIndVal& ret, bool shouldFseek) const {
    // read encoded entry at `pos`
    uchar* entryPtr;
    // note that `entry` must be declared out here for `entryPtr`, which may point to it,
    // to point to a valid address for its whole lifetime
    uchar entry[this->ENTRY_LEN()];
    if (this->SHOULD_BUFFER_READ(oper)) {
        entryPtr = this->readEncoded(oper, pos);
    } else {
        entryPtr = this->readEncodedNoBuf(oper, pos, entry, shouldFseek);
    }
    if (std::memcmp(entryPtr, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    // decode the val part of the entry
    ret = EncIndVal::fromUcstr(entryPtr + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(Oper oper, ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(oper, pos, key.c_str(), this->KEY_LEN());
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(oper, pos, ret);
}


void EncIndBase::write(Oper oper, ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek) {
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
    if (this->SHOULD_BUFFER_WRITE(oper)) {
        this->writeEncoded(oper, pos, encodedEntry.c_str());
    } else {
        this->writeEncodedNoBuf(oper, pos, encodedEntry.c_str(), shouldFseek);
    }
}


void EncIndBase::writeToFirstEmpty(Oper oper, ubigint& pos, const EncIndEntry& encIndEntry) {
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


void EncIndBase::endSetup() {
    Buf* setupBuf = this->getBufFromOper(Oper::SETUP);
    this->flushBufIfNotFlushed(setupBuf);
}


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry encIndEntry;
        // (`Oper::SETUP` here to just get a larger buffer; it shouldn't really matter here)
        this->readEntry(Oper::SETUP, pos, encIndEntry, pos == 0);
        std::cerr << pos << ": " << utils::debug::ustrToHex(encIndEntry.toUstr())
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


bool EncIndBase::advanceUntilMatch(
    Oper oper, ubigint& pos, const uchar* match, int matchLen
) const {
    utils::benchmark::startProfile("advance");
    // need this for wrapping logic later to work!
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one bucket (i.e. `this->getBcktSize()`) at a time to search for it

    // for the first read, we read directly from the file, so that if it turns out we don't need to
    // iterate forward, we skip filling the buffer. this is especially good when buffer is big but
    // enc ind is even bigger, as this avoids large amounts of filling and flushing the buffer at
    // different positions and never using it in between when the enc ind is still mostly empty
    utils::benchmark::startProfile("advance1");
    uchar* currEntryPtr;
    uchar currEntry[this->ENTRY_LEN()];
    currEntryPtr = this->readEncodedNoBuf(oper, pos, currEntry, true);
    //if (this->SHOULD_BUFFER_READ(oper)) {
    //    currEntryPtr = this->readEncoded(oper, pos);
    //} else {
    //    currEntryPtr = this->readEncodedNoBuf(oper, pos, currEntry, true);
    //}
    utils::benchmark::stopProfile("advance1");
    if (std::memcmp(currEntryPtr, match, matchLen) == 0) {
        utils::benchmark::stopProfile("advance");
        return true;
    }

    // if we do need to iterate forward, then fill the buffer if needed and read from it
    // importantly, if we are skipping entries (i.e. `this->getBcktSize() > 1`), then we don't
    // buffer searches as we aren't gonna read most of the buffer anyway, so we get to save filling
    // and flushing it constantly (and searches usually don't need us to iterate forward huge
    // amounts unlike the end of setup phases, so filling such large buffers is especially wasteful)
    bigint positionsChecked = 0;
    do {
        positionsChecked++;
        if (positionsChecked == this->getBcktCount()) {
            utils::benchmark::stopProfile("advance");
            return false;
        }

        pos = (pos + this->getBcktSize()) % this->capacity;
        utils::benchmark::startProfile("advance2");
        if (this->SHOULD_BUFFER_READ(oper)) {
            utils::benchmark::startProfile("advance3");
            currEntryPtr = this->readEncoded(oper, pos);
            utils::benchmark::stopProfile("advance3");
        } else {
            std::cout << "not buf!" << std::endl;
            // also, we don't `fseek()` for this read unless we have wrapped around to the
            // beginning of the file via `pos = ... % this->capacity` or if we are skipping
            // entries (i.e. `this->getBcktSize() > 1`), as otherwise the previous `fread()`
            // should've moved the file pointer to the right pos
            utils::benchmark::startProfile("advance4");
            bool shouldFseek = this->getBcktSize() > 1 || pos < this->getBcktSize();
            currEntryPtr = this->readEncodedNoBuf(oper, pos, currEntry, shouldFseek);
            utils::benchmark::stopProfile("advance4");
        }
        utils::benchmark::stopProfile("advance2");
    } while (std::memcmp(currEntryPtr, match, matchLen) != 0);

    utils::benchmark::stopProfile("advance");
    return true;
}


uchar* EncIndBase::readEncoded(Oper oper, ubigint pos) const {
    utils::benchmark::startProfile("read");
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos);
        bufIndex = 0;
    }

    utils::benchmark::startProfile("buf read");
    uchar* ret = bufToUse->read(bufIndex);
    utils::benchmark::stopProfile("buf read");
    utils::benchmark::stopProfile("read");
    return ret;
}


void EncIndBase::writeEncoded(Oper oper, ubigint pos, const uchar* encodedEntry, bool isInit) {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos, isInit);
        bufIndex = 0;
    }

    utils::benchmark::startProfile("buf write");
    bufToUse->write(bufIndex, encodedEntry);
    utils::benchmark::stopProfile("buf write");
}


uchar* EncIndBase::readEncodedNoBuf(Oper oper, ubigint pos, uchar* ret, bool shouldFseek) const {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        // if `pos` is not covered by buffer, read directly from file; we can completely ignore
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
                std::cerr << "Error: EncIndBase::readEncodedNoBuf(): error reading from file "
                          << this->filename << " (nothing read)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
        return ret;
    } else {
        // if `pos` is covered by the buffer, read it from the buffer instead since the buffer may
        // have a more updated version of that entry than the file
        utils::benchmark::startProfile("buf read");
        uchar* actualRet = bufToUse->read(bufIndex);
        utils::benchmark::stopProfile("buf read");
        return actualRet;
    }
}


void EncIndBase::writeEncodedNoBuf(
    Oper oper, ubigint pos, const uchar* encodedEntry, bool shouldFseek
) {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufFromOper(oper);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        // if `pos` is not covered by buffer, write directly to file; we can completely ignore
        // the buffer here as the buffer does not have an entry to update with this write
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
                std::cerr << "Error: EncIndBase::writeEncodedNoBuf(): error writing to file "
                          << this->filename << " (nothing written)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    } else {
        // if `pos` is covered by the buffer, write it to the buffer instead since the buffer must
        // have the most updated version of that entry. note that reading directly from the file
        // using `readEncodedNoBuf()` should still be correct as either the buffer hasn't
        // changed and `readEncodedNoBuf()` will read from the buffer, or the buffer has
        // changed and has hence been flushed before `readEncodedNoBuf()` reads from the file
        utils::benchmark::startProfile("buf write");
        bufToUse->write(bufIndex, encodedEntry);
        utils::benchmark::stopProfile("buf write");
    }
}


bool EncIndBase::readEntry(Oper oper, ubigint pos, EncIndEntry& ret, bool shouldFseek) const {
    // read encoded entry at `pos`
    uchar* entryPtr;
    uchar entry[this->ENTRY_LEN()];
    if (this->SHOULD_BUFFER_READ(oper)) {
        entryPtr = this->readEncoded(oper, pos);
    } else {
        entryPtr = this->readEncodedNoBuf(oper, pos, entry, shouldFseek);
    }
    if (std::memcmp(entryPtr, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    // decode the entry
    ret = EncIndEntry::fromUcstr(entry, this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


//------------------------------------------------------------------------------
// buffer


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
