#include "utils/types/encr_ind/encr_ind_base.h"

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
#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base_buf.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// constructors/destructors


EncrIndBase::~EncrIndBase() {
    this->clear();
}


//------------------------------------------------------------------------------
// rule of five


void EncrIndBase::copyFrom(const EncrIndBase& other) {
    IDiskStorage<uchar>::copyFrom(other);

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

    if (other.updateBuf != nullptr) {
        this->updateBuf = new Buf(*other.updateBuf);
    } else {
        this->updateBuf = nullptr;
    }
}


void EncrIndBase::moveFrom(EncrIndBase&& other) noexcept {
    IDiskStorage<uchar>::moveFrom(std::move(other));

    // this is now regular pointer assignment instead of actually copying the heap data
    this->NULL_ENTRY = other.NULL_ENTRY;
    other.NULL_ENTRY = nullptr;

    this->capacity = other.capacity;

    this->setupBuf = other.setupBuf;
    other.setupBuf = nullptr;

    this->searchBuf = other.searchBuf;
    other.searchBuf = nullptr;

    this->updateBuf = other.updateBuf;
    other.updateBuf = nullptr;
}


// copy constructor
EncrIndBase::EncrIndBase(const EncrIndBase& other) {
    this->copyFrom(other);
}


// copy assignment operator
EncrIndBase& EncrIndBase::operator =(const EncrIndBase& other) {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->copyFrom(other);
    }
    return *this;
}


// move constructor
EncrIndBase::EncrIndBase(EncrIndBase&& other) noexcept {
    this->moveFrom(std::move(other));
}


// move assignment operator
EncrIndBase& EncrIndBase::operator =(EncrIndBase&& other) noexcept {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->moveFrom(std::move(other));
    }
    return *this;
}


//------------------------------------------------------------------------------
// interface


void EncrIndBase::init(SseOper setupOper, bigint capacity) {
    assert(setupOper == SseOper::SETUP || setupOper == SseOper::UPDATE);

    // inits encr ind file and file pointer
    IDiskStorage<uchar>::init();

    // also initialize `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()` relies on virtual methods
    // (technically it is possible that an encrypted tuple happens to be all '0' bytes and thus gets
    // mistaken for a null kv pair, but currently `this->ENTRY_LEN()` is >1000 bits so there's
    // a 2^{>1000} chance of this happening...and USENIX'24's implementation just does this too)
    this->NULL_ENTRY = new uchar[this->ENTRY_LEN()] {};
    this->capacity = capacity;

    // init buffers
    bigint setupBufEntryCapacity = std::min(config::ENC_IND_SETUP_BUF_CAPAC, this->capacity);
    // if our encr ind does not fit entirely in memory, use a smaller buffer to avoid rapid
    // moving (i.e. flushing and refilling) of huge setup buffers
    if (setupBufEntryCapacity < this->capacity) {
        setupBufEntryCapacity = std::min(config::ENC_IND_SETUP_OVERFLOW_BUF_CAPAC, this->capacity);
    }
    this->setupBuf = new Buf(
        setupBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    // heuristically determine this size
    bigint searchBufEntryCapacity = utils::misc::roundUpToPowOf2(this->capacity / std::pow(2, 9));
    searchBufEntryCapacity = std::min(searchBufEntryCapacity, config::ENC_IND_SEARCH_BUF_MAX_CAPAC);
    searchBufEntryCapacity = std::min(searchBufEntryCapacity, this->capacity);
    if (this->capacity > 0) {
        // buffer must have nonzero size (0 is possible due to `/ std::pow(...)`) to avoid errors
        searchBufEntryCapacity = std::max(searchBufEntryCapacity, (bigint)1);
    }
    this->searchBuf = new Buf(
        searchBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    bigint updateBufEntryCapacity = std::min(config::ENC_IND_UPDATE_BUF_CAPAC, this->capacity);
    this->updateBuf = new Buf(
        updateBufEntryCapacity, this->file, this->filename, this->capacity, this->ENTRY_LEN()
    );

    // fill file with zero bits, so we can tell if a spot is empty by if it contains all zero bits
    // and can use buffer to speed this up (although this seems to only be efficient at big sizes)
    for (bigint i = 0; i < this->capacity; i++) {
        // we allow incomplete buffer fills from the file here since, well, the file is incomplete
        this->writeEncoded(setupOper, i, this->NULL_ENTRY, true);
    }
    // this is needed so that the buf does not contain a region where the file simply does not
    // have yet, which makes `NoBuf` methods later wrong!
    this->flushBufIfNotFlushed(this->getBufForSseOper(setupOper));
}


void EncrIndBase::clear() {
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
    if (this->updateBuf != nullptr) {
        delete this->updateBuf;
        this->updateBuf = nullptr;
    }

    // clears DB file and file pointer
    IDiskStorage<uchar>::clear();
}


bool EncrIndBase::read(SseOper oper, ubigint pos, EncrIndVal& ret, bool shouldFseek) const {
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
    ret = EncrIndVal::decode(entryPtr + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncrIndBase::find(SseOper oper, ubigint& pos, const ustring& key, EncrIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(oper, pos, key.c_str(), this->KEY_LEN());
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(oper, pos, ret, true);
}


void EncrIndBase::write(
    SseOper oper, ubigint pos, const EncrIndEntry& encrIndEntry, bool shouldFseek
) {
    // encode `encrIndEntry`
    ustring encodedEntry = encrIndEntry.encode();
    DEBUG_ONLY({
        if (encodedEntry.length() != this->ENTRY_LEN()) {
            std::cerr << "Error: EncrIndBase::write(): write of length " << encodedEntry.length()
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


void EncrIndBase::writeToFirstEmpty(SseOper oper, ubigint& pos, const EncrIndEntry& encrIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(oper, pos, this->NULL_ENTRY, this->ENTRY_LEN());
    // if we've scoured the whole index and still haven't found an available space,
    // throw an error: we are trying to write to a full index
    DEBUG_ONLY({
        if (!isEmptyAvailable) {
            std::cerr << "Error: EncrIndBase::writeToFirstEmpty(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // write into the empty location we found
    this->write(oper, pos, encrIndEntry, true);
}


void EncrIndBase::endSetup(SseOper setupOper) {
    assert(setupOper == SseOper::SETUP || setupOper == SseOper::UPDATE);
    // make sure the changes in the setup buffer are visible to the separate search buffer!
    Buf* buf = this->getBufForSseOper(setupOper);
    this->flushBufIfNotFlushed(buf);
}


//------------------------------------------------------------------------------
// helpers


bool EncrIndBase::advanceUntilMatch(
    SseOper oper, ubigint& pos, const uchar* match, int matchLen
) const {
    // need this for wrapping logic later to work!
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one bucket (i.e. `this->getBcktSize()`) at a time to search for it

    // for the first read, we skip filling the buffer if it turns out we don't need to iterate
    // forward. this is especially good when the buffer is big but the encr ind is even bigger,
    // as this avoids large amounts of filling and flushing the buffer at different positions
    // and never using it in between (especially when the encr ind is still mostly empty)
    uchar* currEntryPtr;
    uchar currEntry[this->ENTRY_LEN()];
    currEntryPtr = this->readEncodedOptionalBuf(oper, pos, currEntry, true);
    if (std::memcmp(currEntryPtr, match, matchLen) == 0) {
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
            return false;
        }

        pos = (pos + this->getBcktSize()) % this->capacity;
        if (this->SHOULD_BUFFER_ADVANCE(oper)) {
            currEntryPtr = this->readEncoded(oper, pos);
        } else {
            // also, we don't `fseek()` for this read unless we have wrapped around to the
            // beginning of the file via `pos = ... % this->capacity` or if we are skipping
            // entries (i.e. `this->getBcktSize() > 1`), as otherwise the previous `fread()`
            // should've moved the file pointer to the right pos
            bool shouldFseek = this->getBcktSize() > 1 || pos < this->getBcktSize();
            currEntryPtr = this->readEncodedNoBuf(oper, pos, currEntry, shouldFseek);
        }
    } while (std::memcmp(currEntryPtr, match, matchLen) != 0);

    return true;
}


uchar* EncrIndBase::readEncoded(SseOper oper, ubigint pos) const {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufForSseOper(oper);
    assert(bufToUse != nullptr);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos);
        bufIndex = 0;
    }

    return bufToUse->read(bufIndex);
}


void EncrIndBase::writeEncoded(SseOper oper, ubigint pos, const uchar* encodedEntry, bool isInit) {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufForSseOper(oper);
    assert(bufToUse != nullptr);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        this->fillBuf(bufToUse, pos, isInit);
        bufIndex = 0;
    }

    bufToUse->write(bufIndex, encodedEntry);
}


uchar* EncrIndBase::readEncodedOptionalBuf(
    SseOper oper, ubigint pos, uchar* ret, bool shouldFseek
) const {
    pos %= this->capacity;

    Buf* bufToUse = this->getBufForSseOper(oper);
    assert(bufToUse != nullptr);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex == Buf::NOT_IN_BUF) {
        // if `pos` is not covered by buffer, read directly from file; we can completely ignore
        // the buffer here as the file must have the most updated copy of the entry at `pos`
        assert(this->file != nullptr);
        if (shouldFseek) {
            std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        }
        this->readFromFile(ret, this->ENTRY_LEN(), 1, "EncrIndBase::readEncodedOptionalBuf()");
        return ret;
    } else {
        // if `pos` is covered by the buffer, read it from the buffer instead since the buffer may
        // have a more updated version of that entry than the file

        // IMPORTANT: this does NOT guarantee that the file pointer is moved correctly! as this is
        // transparently an "optional" buf read, the caller bears the responsibility of checking
        return bufToUse->read(bufIndex);
    }
}


uchar* EncrIndBase::readEncodedNoBuf(
    SseOper oper, ubigint pos, uchar* ret, bool shouldFseek
) const {
    assert(this->file != nullptr);
    pos %= this->capacity;

    // read encoded entry from file into `ret` parameter
    if (shouldFseek) {
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    }
    // note that we do still need the check for no items read in `this->readFromFile()` even if
    // we end up reading from the buffer instead, as otherwise the caller may think the read
    // from file succeeded and not call for an `fseek()` subsequently when in reality the failed
    // `fread()` didn't move the file pointer
    //
    // this is also why we needed to flush the buffer at the end of `init()`!
    this->readFromFile(ret, this->ENTRY_LEN(), 1, "EncrIndBase::readEncodedNoBuf()");

    // also read encoded entry from buffer if `pos` is covered by the buffer, as it must have
    // the most updated version of that entry, and return it instead of the `ret` parameter
    Buf* bufToUse = this->getBufForSseOper(oper);
    assert(bufToUse != nullptr);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex != Buf::NOT_IN_BUF) {
        return bufToUse->read(bufIndex);
    } else {
        // if we are returning what we read from the file
        return ret;
    }
}


void EncrIndBase::writeEncodedNoBuf(
    SseOper oper, ubigint pos, const uchar* encodedEntry, bool shouldFseek
) {
    assert(this->file != nullptr);
    pos %= this->capacity;

    // write encoded entry to file
    if (shouldFseek) {
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    }
    this->writeToFile(encodedEntry, this->ENTRY_LEN(), 1, "EncrIndBase::writeEncodedNoBuf()");

    // also write encoded entry to buffer if `pos` is covered by the buffer, to ensure that
    // the buffer has the most updated version of that entry
    Buf* bufToUse = this->getBufForSseOper(oper);
    assert(bufToUse != nullptr);
    bigint bufIndex = this->posToBufIndex(bufToUse, pos);
    if (bufIndex != Buf::NOT_IN_BUF) {
        bufToUse->write(bufIndex, encodedEntry);
    }
}


//------------------------------------------------------------------------------
// buffer


void EncrIndBase::fillBuf(Buf* buf, ubigint bufStartPos, bool isEncrIndInit) const {
    assert(buf != nullptr);
    this->flushBufIfNotFlushed(buf);
    buf->fill(bufStartPos, isEncrIndInit);
}


void EncrIndBase::flushBufIfNotFlushed(Buf* buf) const {
    assert(buf != nullptr);
    buf->flushIfNotFlushed();
    this->isFlushed = false;
    // remember to then flush the fwrite buffer to the file too
    this->flushIfNotFlushed();
}


bigint EncrIndBase::posToBufIndex(Buf* buf, ubigint pos) const {
    assert(buf != nullptr);
    return buf->posToBufIndex(pos);
}


//------------------------------------------------------------------------------
// debugging


void EncrIndBase::printBuf(SseOper oper) const {
    Buf* buf = this->getBufForSseOper(oper);
    assert(buf != nullptr);
    if (!buf->isFilled) {
        std::cerr << "Buf claims to be unfilled; garbage data may be produced!" << std::endl;
    }
    for (bigint pos = 0; pos < buf->ENTRY_CAPACITY; pos++) {
        uchar* currEntry = buf->read(pos);
        std::cerr << pos << ": " << utils::debug::ustrToHex(currEntry, this->ENTRY_LEN())
                  << std::endl;
    }
}


void EncrIndBase::printFile() const {
    assert(this->file != nullptr);
    bigint origFilePtrPos = std::ftell(this->file);
    uchar currEntry[this->ENTRY_LEN()];
    for (bigint pos = 0; pos < this->capacity; pos++) {
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }

        this->readFromFile(currEntry, this->ENTRY_LEN(), 1, "EncrIndBase::printFile()");
        std::cerr << pos << ": " << utils::debug::ustrToHex(currEntry, this->ENTRY_LEN())
                  << std::endl;
    }
    std::fseek(this->file, origFilePtrPos, SEEK_SET);
}
