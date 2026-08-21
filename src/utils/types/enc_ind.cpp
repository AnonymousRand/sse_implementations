#include "utils/types/enc_ind.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "config.h"

#include "utils/benchmark.h"
#include "utils/debugging.h"
#include "utils/types/basic_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


//==============================================================================
// utils
//==============================================================================


namespace utils::enc_ind {


ustring toUstr(const EncIndEntry& encIndEntry) {
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    return key + val.first + val.second;
}


} // namespace `utils::enc_ind`


//==============================================================================
// `EncIndBase`
//==============================================================================


// this initializes `NULL_ENTRY` to a contiguous block of zero bits
// (technically it is possible that some encrypted tuple happened to be all `0` bytes
// and thus get mistaken for a null kv pair, but currently `ENTRY_LEN` is in the
// thousands of bits so there's a 2^{>1000} chance of this happening...and USENIX'24's
// implementation seems to just do this too)
const uchar EncIndBase::NULL_ENTRY[ENTRY_LEN] = {};


//------------------------------------------------------------------------------
// constructors/destructors


EncIndBase::EncIndBase(std::shared_ptr<Benchmark> benchmark) : benchmark(benchmark) {}


//------------------------------------------------------------------------------
// the big five


// copy constructor
EncIndBase::EncIndBase(const EncIndBase& other) {
    IDiskStorage::copyFrom(other);
}


//------------------------------------------------------------------------------
// interface


void EncIndBase::init(bigint size) {
    // inits DB file and file pointer
    IDiskStorage::init();

    this->size = size;

    // fill file with zero bits
    this->benchmark->startProfile("init");
    for (bigint i = 0; i < this->size; i++) {
        int itemsWritten = std::fwrite(NULL_ENTRY, ENTRY_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error: EncIndBase::init(): error initializing file " << this->filename
                      << " with zero bits (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::fflush(this->file);
    this->benchmark->stopProfile("init");
}


void EncIndBase::clear() {
    this->size = 0;

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(ubigint pos, EncIndVal& ret) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readRaw(entry);
    if (std::memcmp(entry, NULL_ENTRY, ENTRY_LEN) == 0) {
        // if `pos` contains `NULL_ENTRY`
        return false;
    }

    ret.first = ustring(&entry[KEY_LEN], DATA_LEN);
    ret.second = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return true;
}


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    // encode `encIndEntry` into one string
    this->benchmark->startProfile("encode");
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring entry = key + val.first + val.second;
    if (entry.length() != ENTRY_LEN) {
        std::cerr << "Error: EncIndBase::write(): write of length " << entry.length()
                  << " bytes is not allowed! (want " << ENTRY_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->benchmark->stopProfile("encode");

    // then go to `pos` and write the encoded `encIndEntry`
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->benchmark->startProfile("fwrite");
    int itemsWritten = std::fwrite(entry.c_str(), ENTRY_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: EncIndBase::write(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->benchmark->stopProfile("fwrite");
    // flush immediately to mark space as occupied (and so we always have `this->isFlushed = true`)
    this->benchmark->startProfile("fflush");
    std::fflush(this->file);
    this->benchmark->stopProfile("fflush");
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(pos, key.c_str(), KEY_LEN);
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(pos, NULL_ENTRY, ENTRY_LEN);
    // if we've scoured the whole index and still haven't found an available space,
    // throw an error: we are trying to write to a full index
    if (!isEmptyAvailable) {
        std::cerr << "Error: EncIndBase::writeToFirstEmpty(): ran out of space writing to "
                  << this->filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // write into the empty location we found
    this->write(pos, encIndEntry);
}


//------------------------------------------------------------------------------
// helpers


void EncIndBase::readRaw(uchar* buf) const {
    this->benchmark->startProfile("fread");
    bigint itemsRead = std::fread(buf, ENTRY_LEN, 1, this->file);
    this->benchmark->stopProfile("fread");
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::readRaw(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


//------------------------------------------------------------------------------
// debugging


EncIndEntry EncIndBase::getEncIndEntry(ubigint pos) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readRaw(entry);

    ustring key = ustring(&entry[0], KEY_LEN);
    ustring data = ustring(&entry[KEY_LEN], DATA_LEN);
    ustring iv = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return EncIndEntry {key, EncIndVal {data, iv}};
};


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->size; pos++) {
        EncIndEntry entry = this->getEncIndEntry(pos);
        std::cerr << pos << ": " << utils::debugging::ustrToHex(utils::enc_ind::toUstr(entry))
                  << std::endl << std::endl;
    }
}


//==============================================================================
// `EncIndRand`
//==============================================================================


//------------------------------------------------------------------------------
// helpers


bool EncIndRand::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->size;

    // get entry at `pos`, and if it doesn't match `match` (e.g. because of `pos %= this->size`),
    // iterate forward one position at a time to search for it
    const ubigint origStartPos = pos;
    const bigint readBufEntryCapacity = std::min(config::ENC_IND_READ_BUF_CAPACITY, this->size);
    uchar readBuf[readBufEntryCapacity * ENTRY_LEN];
    bigint readBufEntryCount = this->readIntoReadBuf(
        readBuf, readBufEntryCapacity, pos, origStartPos, true
    );
    bigint readBufIndex = 0;
    bool needsFseek = false;
    bigint positionsChecked = 0;
    while (std::memcmp(readBuf + (readBufIndex * ENTRY_LEN), match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->size) {
            return false;
        }

        pos = (pos + 1) % this->size;
        if (pos == 0) {
            // the file pointer should be in the right position from the last `fread()` into
            // `readBuf` (and hence doesn't need an `fseek()`) IF AND ONLY IF we do not wrap around
            // (assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
            // (also, this can't be just an `fseek(0)` call here since we may only need to `fread()`
            // to fill `readBuf` again later on, when we need to read pointer to not still be at 0)
            needsFseek = true;
        }

        // (this must come before we set `readBufIndex` to 0, or else we skip over an entry)
        readBufIndex++;
        // if we've read to the end of the current `readBuf`, read the next part of the file into it
        if (readBufIndex >= readBufEntryCount) {
            readBufEntryCount = this->readIntoReadBuf(
                readBuf, readBufEntryCapacity, pos, origStartPos, needsFseek
            );
            readBufIndex = 0;
            needsFseek = false;
        }
    }

    return true;
}


bigint EncIndRand::readIntoReadBuf(
    uchar* readBuf, bigint targetEntryCount, ubigint readBufStartPos, ubigint origStartPos,
    bool needsFseek
) const {
    bigint entriesUntilEof = this->size - readBufStartPos;
    bigint entriesUntilFullLoop;
    if (readBufStartPos < origStartPos)      entriesUntilFullLoop = origStartPos - readBufStartPos;
    else if (readBufStartPos > origStartPos) entriesUntilFullLoop = entriesUntilEof + origStartPos;
    else                                     entriesUntilFullLoop = this->size;
    // we want to make sure we don't exceed where we had started doing this whole thing back in
    // the caller (e.g. if we had already wrapped around and are getting close to a full loop)
    bigint entriesToRead = std::min(targetEntryCount, entriesUntilFullLoop);

    bigint entriesToReadUntilEof = std::min(entriesToRead, entriesUntilEof);
    this->benchmark->startProfile("fseek");
    if (needsFseek) {
        std::fseek(this->file, readBufStartPos * ENTRY_LEN, SEEK_SET);
    }
    this->benchmark->stopProfile("fseek");
    this->benchmark->startProfile("fread");
    bigint itemsRead = std::fread(readBuf, ENTRY_LEN, entriesToReadUntilEof, this->file);
    this->benchmark->stopProfile("fread");
    if (itemsRead < entriesToReadUntilEof) {
        std::cerr << "Error: EncIndRand::readIntoReadBuf(): error reading (part 1) "
                  << "from file " << this->filename
                  << " (only read " << itemsRead << " out of " << entriesToReadUntilEof << ")"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // wrap around to beginning of file if we read less than the target number of entries
    if (entriesToReadUntilEof < entriesToRead) {
        this->benchmark->startProfile("fseek");
        std::fseek(this->file, 0, SEEK_SET);
        this->benchmark->stopProfile("fseek");
        this->benchmark->startProfile("fread");
        itemsRead += std::fread(
            readBuf + (entriesToReadUntilEof * ENTRY_LEN),
            ENTRY_LEN, entriesToRead - entriesToReadUntilEof,
            this->file
        );
        this->benchmark->stopProfile("fread");
        if (itemsRead < entriesToRead) {
            std::cerr << "Error: EncIndRand::writeToFirstEmpty(): error reading (part 2) "
                      << "from file " << this->filename
                      << " (only read " << itemsRead << " out of " << entriesToRead << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    
    return itemsRead;
}


//==============================================================================
// `EncIndLoc`
//==============================================================================


//------------------------------------------------------------------------------
// interface


void EncIndLoc::init(bigint bcktSize, bigint bcktCount) {
    EncIndBase::init(bcktSize * bcktCount);

    this->bcktSize = bcktSize;
    this->bcktCount = bcktCount;
}


void EncIndLoc::clear() {
    EncIndBase::clear();

    this->bcktSize = 0;
    this->bcktCount = 0;
}


//------------------------------------------------------------------------------
// helpers


bool EncIndLoc::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->size;

    // get entry at `pos`, and if it doesn't match `match` (e.g. because of `pos %= this->size`),
    // iterate forward by `this->bcktSize` positions at a time to search for it
    // 
    // importantly, we get the massive optimization of only having to check the first entry of every
    // bucket/every `this->bcktSize` entries, as locality guarantees contiguousness of buckets
    uchar currEntry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readRaw(currEntry);
    bigint positionsChecked = 0;
    while (std::memcmp(currEntry, match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->bcktCount) {
            return false;
        }

        pos = (pos + this->bcktSize) % this->size;
        // if either we need to `fseek()` to further than we had `fread()` (i.e.
        // `this->bcktSize` > 1), or `pos` had been decreased this iteration (i.e.
        // `pos < this->bcktSize`) meaning we must've wrapped around, call `fseek()`
        // to make sure we are on the correct position (otherwise the previous `fread()`
        // automatically handles it, so we can save some time)
        if (this->bcktSize > 1 || pos < this->bcktSize) {
            this->benchmark->startProfile("fseek");
            std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
            this->benchmark->stopProfile("fseek");
        }

        this->readRaw(currEntry);
    }
    
    return true;
}
