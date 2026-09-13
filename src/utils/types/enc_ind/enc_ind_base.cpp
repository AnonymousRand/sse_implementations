#include "utils/types/enc_ind/enc_ind_base.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include "config.h"

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/misc.h"
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
    this->filledCount = other.filledCount;
}


void EncIndBase::moveFrom(EncIndBase&& other) noexcept {
    IDiskStorage::moveFrom(std::move(other));

    // this is now regular pointer assignment instead of actually copying the heap data
    this->NULL_ENTRY = other.NULL_ENTRY;
    other.NULL_ENTRY = nullptr;

    this->capacity = other.capacity;
    this->filledCount = other.filledCount;
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

    // this initializes `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()` relies on virtual methods
    // (technically it is possible that an encrypted tuple happens to be all '0' bytes and thus gets
    // mistaken for a null kv pair, but currently `this->ENTRY_LEN()` is >1000 bits so there's
    // a 2^{>1000} chance of this happening...and USENIX'24's implementation just does this too)
    this->NULL_ENTRY = new uchar[this->ENTRY_LEN()] {};
    this->capacity = capacity;

    // fill file with zero bits
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
}


void EncIndBase::clear() {
    if (this->NULL_ENTRY != nullptr) {
        delete[] this->NULL_ENTRY;
        this->NULL_ENTRY = nullptr;
    }
    this->capacity = 0;

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(ubigint pos, EncIndVal& ret, bool shouldFseek) const {
    // read encoded entry from `pos`
    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(pos, entry, shouldFseek);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    // decode entry
    ret = EncIndVal::fromUcstr(entry + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(
        pos, key.c_str(), this->KEY_LEN(), config::SHOULD_BUFFER_SEARCH
    );
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek) {
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

    // write encoded `encIndEntry` to `pos`
    this->writeEncoded(pos, encodedEntry.c_str(), shouldFseek);
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(
        pos, this->NULL_ENTRY, this->ENTRY_LEN(), config::SHOULD_BUFFER_SETUP
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


void EncIndBase::readEncoded(ubigint pos, uchar* buf, bool shouldFseek) const {
    pos %= this->capacity;
    this->flushIfNotFlushed();

    if (shouldFseek) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
    }
    utils::benchmark::startProfile("fread");
    bigint itemsRead = std::fread(buf, this->ENTRY_LEN(), 1, this->file);
    utils::benchmark::stopProfile("fread");
    DEBUG_ONLY({
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndBase::readEncoded(): error reading from file "
                      << this->filename << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
}


void EncIndBase::writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek) {
    pos %= this->capacity;

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
    this->filledCount++;
}


bool EncIndBase::advanceUntilMatch(
    ubigint& pos, const uchar* match, int matchLen, bool shouldBuffer
) const {
    // need this for wrapping logic later to work!
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward `this->getBcktSize()` positions at a time to search for it
    uchar currEntry[this->ENTRY_LEN()];
    this->readEncoded(pos, currEntry, true);
    if (std::memcmp(currEntry, match, matchLen) == 0 || this->capacity == 0) {
        return true;
    }

    if (shouldBuffer && this->getBcktSize() <= 1) {
        // if we do need to iterate forward, we can use a buffer in memory to speed up long chains
        // of iterating forward a small number of (i.e. `this->getBcktSize()`) positions at a time
        assert(this->capacity != 0);
        double fillPercentage = this->filledCount / (double)this->capacity;
        // this is a heuristic formula derived from printing out the actual positions checked
        // by fill percentage, finding an exponential line of best fit, playing around in desmos,
        // rearranging equations, testing, and sleep deprivation
        bigint readBufEntryCapacity = std::ceil(
            //std::pow(this->capacity, 4 * fillPercentage - 3) * std::pow(2, -11 * fillPercentage + 5)
            std::pow(this->capacity, 4 * fillPercentage - 3) * std::pow(2, -11 * fillPercentage + 3)
        );
        readBufEntryCapacity = utils::misc::roundUpToPowOf2(readBufEntryCapacity);
        readBufEntryCapacity = std::max(readBufEntryCapacity, (bigint)1);
        readBufEntryCapacity = std::min(
            readBufEntryCapacity, config::ENC_IND_MAX_READ_BUF_CAPACITY
        );
        readBufEntryCapacity = std::min(readBufEntryCapacity, this->capacity);

        const ubigint origStartPos = pos;
        // this has to be on the heap since it may overflow the stack
        uchar* readBuf = new uchar[readBufEntryCapacity * this->ENTRY_LEN()];
        bigint readBufEntryCount = this->readIntoReadBuf(
            // technically we are repeating the first read again, but doing `+ 1` breaks so whatever
            readBuf, readBufEntryCapacity, pos, origStartPos, true
        );
        bigint readBufIndex = 0;
        bool needsFseek = false;
        bigint positionsChecked = 0;
        while (std::memcmp(readBuf + (readBufIndex * this->ENTRY_LEN()), match, matchLen) != 0) {
            positionsChecked++;
            if (positionsChecked == this->getBcktCount()) {
                delete[] readBuf;
                return false;
            }

            pos = (pos + this->getBcktSize()) % this->capacity;
            if (this->getBcktSize() > 1 || pos < this->getBcktSize()) {
                // if either we need to `fseek()` to further than we had `fread()` (i.e.
                // `this->getBcktSize()` > 1), or `pos` had been decreased this iteration (i.e.
                // `pos < this->getBcktSize()`) meaning we must've wrapped around, call `fseek()`
                // to make sure we are on the correct position (otherwise the previous `fread()`
                // automatically handles it, so we can save some time)
                needsFseek = true;
            }

            // (this must come before we set `readBufIndex` to 0, or else we skip over an entry)
            readBufIndex += this->getBcktSize();
            // if we've read to the end of `readBuf`, read the next part of the file into it
            if (readBufIndex >= readBufEntryCount) {
                readBufEntryCount = this->readIntoReadBuf(
                    readBuf, readBufEntryCapacity, pos, origStartPos, needsFseek
                );
                readBufIndex = 0;
                needsFseek = false;
            }
        }

        delete[] readBuf;
    } else {
        // buffer-less iterating
        bigint positionsChecked = 0;
        while (std::memcmp(currEntry, match, matchLen) != 0) {
            positionsChecked++;
            if (positionsChecked == this->getBcktCount()) {
                return false;
            }

            pos = (pos + this->getBcktSize()) % this->capacity;
            if (this->getBcktSize() > 1 || pos < this->getBcktSize()) {
                this->readEncoded(pos, currEntry, true);
            } else {
                this->readEncoded(pos, currEntry, false);
            }
        }
    }

    return true;
}


bool EncIndBase::readEntry(ubigint pos, EncIndEntry& ret, bool shouldFseek) const {
    uchar entry[this->ENTRY_LEN()];
    this->readEncoded(pos, entry, shouldFseek);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndEntry::fromUcstr(entry, this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bigint EncIndBase::readIntoReadBuf(
    uchar* readBuf, bigint targetEntryCount, ubigint readBufStartPos, ubigint origStartPos,
    bool needsFseek
) const {
    bigint entriesUntilEof = this->capacity - readBufStartPos;
    bigint entriesUntilFullLoop;
    if      (readBufStartPos < origStartPos) entriesUntilFullLoop = origStartPos - readBufStartPos;
    else if (readBufStartPos > origStartPos) entriesUntilFullLoop = entriesUntilEof + origStartPos;
    else                                     entriesUntilFullLoop = this->capacity;
    // we want to make sure we don't exceed where we had started doing this whole thing back in
    // the caller (e.g. if we had already wrapped around and are getting close to a full loop)
    bigint entriesToRead = std::min(targetEntryCount, entriesUntilFullLoop);

    // first read as much of the target entry count as we can without exceeding EOF
    bigint entriesToRead1 = std::min(entriesToRead, entriesUntilEof);
    if (needsFseek) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, readBufStartPos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
    }
    utils::benchmark::startProfile("fread");
    bigint itemsRead = std::fread(readBuf, this->ENTRY_LEN(), entriesToRead1, this->file);
    utils::benchmark::stopProfile("fread");
    DEBUG_ONLY({
        if (itemsRead < entriesToRead1) {
            std::cerr << "Error: EncIndBase::readIntoReadBuf(): error reading (part 1) "
                      << "from file " << this->filename
                      << " (only read " << itemsRead << " out of " << entriesToRead1 << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // then wrap around to beginning of file if we read less than the target number of entries
    if (entriesToRead1 < entriesToRead) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, 0, SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        utils::benchmark::startProfile("fread");
        itemsRead += std::fread(
            readBuf + (entriesToRead1 * this->ENTRY_LEN()),
            this->ENTRY_LEN(), entriesToRead - entriesToRead1,
            this->file
        );
        utils::benchmark::stopProfile("fread");
        DEBUG_ONLY({
            if (itemsRead < entriesToRead) {
                std::cerr << "Error: EncIndBase::readIntoReadBuf(): error reading (part 2) "
                          << "from file " << this->filename
                          << " (only read " << itemsRead << " out of " << entriesToRead << ")"
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
    
    return itemsRead;
}
