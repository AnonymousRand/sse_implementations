#include "utils/types/enc_ind.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

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
// `IDiskStorage`


void EncIndBase::init(bigint size) {
    // inits DB file and file pointer
    IDiskStorage::init();

    this->size = size;

    // fill file with zero bits
    for (bigint i = 0; i < this->size; i++) {
        int itemsWritten = std::fwrite(NULL_ENTRY, ENTRY_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error: EncIndBase::init(): error initializing file " << this->filename
                      << " with zero bits (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::fflush(this->file);
}


void EncIndBase::clear() {
    this->size = 0;

    // clears DB file and file pointer
    IDiskStorage::clear();
}


//------------------------------------------------------------------------------
// other interface


bool EncIndBase::read(ubigint pos, EncIndVal& ret) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::read(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
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
    this->benchmark->startProfile("fwrite");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
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


//------------------------------------------------------------------------------
// helpers


bool EncIndBase::findBase(
    ubigint& pos, const ustring& key, EncIndVal& ret,
    bigint collisionSkip, bigint collisionAttempts
) const {
    pos %= this->size;

    // get entry at `pos`
    uchar currEntry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::findBase(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if entry at `pos` did not match the target `key` (i.e. another kv pair overflowed here
    // first), scan subsequent locations for where the target `key` could've overflowed to
    const uchar* targetKeyCstr = key.c_str();
    bigint positionsChecked = 0;
    while (std::memcmp(currEntry, targetKeyCstr, KEY_LEN) != 0) {
        positionsChecked++;
        // if still not found after `collisionAttempts` iterations forward, give up
        if (positionsChecked == collisionAttempts) {
            return false;
        }

        pos = (pos + collisionSkip) % this->size;
        // optimization: if we are only iterating forward one position at a time, the file
        // pointer from the previous `fread` does not need to be moved unless we have wrapped
        // around to the beginning, so we don't need extra `fseek()` in that case
        // (this, like, triples the speed of PiBas)
        if (collisionSkip != 1) {
            std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
        } else {
            if (pos == 0) {
                std::fseek(this->file, 0, SEEK_SET);
            }
        }
        itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndBase::findBase(): error reading from file " << this->filename
                      << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


//------------------------------------------------------------------------------
// debugging


EncIndEntry EncIndBase::get(ubigint pos) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::get(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ustring key = ustring(&entry[0], KEY_LEN);
    ustring data = ustring(&entry[KEY_LEN], DATA_LEN);
    ustring iv = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return EncIndEntry {key, EncIndVal {data, iv}};
};


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->size; pos++) {
        EncIndEntry entry = this->get(pos);
        std::cerr << pos << ": " << utils::debugging::ustrToHex(utils::enc_ind::toUstr(entry))
                  << std::endl << std::endl;
    }
}


//==============================================================================
// `EncIndRand`
//==============================================================================


void EncIndRand::writeToFirstEmpty(ubigint pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    // first check if location at `pos` is already filled (e.g. because of `pos %= this->size`)
    // if it is, find the next available location, iterating forward by `collisionSkip` positions
    // at a time (this is also what USENIX'24's implementation does)
    // >TODO if this works out, do similar optimizations for `findBase()`? or no since that is
    // where locality shines?
    // TODO: add fast setup config option and this buffer size
    const ubigint origStartPos = pos;
    constexpr bigint READ_BUF_TARGET_ENTRIES = std::pow(2, 9);
    const bigint readBufEntryCapacity = std::min(READ_BUF_TARGET_ENTRIES, this->size);
    uchar readBuf[readBufEntryCapacity * ENTRY_LEN];
    bigint readBufEntryCount = 0;
    bigint readBufIndex = 0;
    bigint positionsChecked = 0;

    this->benchmark->startProfile("fread");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    readBufEntryCount = this->readIntoReadBuf(readBuf, readBufEntryCapacity, pos, origStartPos);
    this->benchmark->stopProfile("fread");
    // note: the file pointer should be in the right position from the last `fread()`
    // into `readBuf` IF AND ONLY IF `collisionSkip` <= `readBufEntryCount`, i.e.
    // we don't advance `pos` by more than the size of the entire buffer at a time
    // (assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
    this->benchmark->startProfile("fread2");
    // >>TODO if current way of separating rand and loc writeToFirstEmpty() is faster, try to
    // refactor this to a do-while loop as well, and then still share some common code in EncIndBase
    while (std::memcmp(readBuf + (readBufIndex * ENTRY_LEN), NULL_ENTRY, ENTRY_LEN) != 0) {
        positionsChecked++;
        // if we've done `collisionAttempts` iterations and still haven't found an available space,
        // throw an error (this usually means we are trying to write to a full index)
        if (positionsChecked == this->size) {
            std::cerr << "Error: EncIndRand::writeToFirstEmpty(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // this should be the only place we handle updating `pos`
        readBufIndex++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }

        // if we've gotten to the end of the current `readBuf`, read the next part of the file
        // into it, and also reset its internal `readBufIndex` index
        if (readBufIndex >= readBufEntryCount) {
            readBufEntryCount = this->readIntoReadBuf(
                readBuf, readBufEntryCapacity, pos, origStartPos
            );
            readBufIndex = 0;
        }
    }
    this->benchmark->stopProfile("fread2");

    // write into the empty location we found
    this->write(pos, encIndEntry);
}


bigint EncIndRand::readIntoReadBuf(
    uchar* readBuf, bigint targetEntryCount, ubigint readBufStartPos, ubigint origStartPos
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
    this->benchmark->startProfile("fread3");
    bigint itemsRead = std::fread(readBuf, ENTRY_LEN, entriesToReadUntilEof, this->file);
    this->benchmark->stopProfile("fread3");
    if (itemsRead < entriesToReadUntilEof) {
        std::cerr << "Error: EncIndRand::readIntoReadBuf(): error reading (part 1) "
                  << "from file " << this->filename
                  << " (only read " << itemsRead << " out of " << entriesToReadUntilEof << ")"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // wrap around to beginning of file if we read less than the target number of entries
    if (entriesToReadUntilEof < entriesToRead) {
        std::fseek(this->file, 0, SEEK_SET);
        itemsRead += std::fread(
            readBuf + (entriesToReadUntilEof * ENTRY_LEN),
            ENTRY_LEN, entriesToRead - entriesToReadUntilEof,
            this->file
        );
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


void EncIndLoc::writeToFirstEmpty(
    ubigint& pos, const EncIndEntry& encIndEntry, bigint collisionSkip, bigint collisionAttempts
) {
    pos %= this->size;

    // first check if location at `pos` is already filled (e.g. because of `pos %= this->size`)
    // if it is, find the next available location, iterating forward by `collisionSkip` positions
    // at a time (this is also what USENIX'24's implementation does)
    //readBufEntryCount = this->readIntoReadBuf(
    //    readBuf, readBufEntryCapacity, pos, origStartPos, true
    //);
    //this->benchmark->stopProfile("fread");
    uchar currEntry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    bigint positionsSeen = 0;
    this->benchmark->startProfile("fread2");
    do {
        // if we've done `collisionAttempts` iterations and still haven't found an available space,
        // throw an error (this usually means we are trying to write to a full index)
        if (positionsSeen == collisionAttempts) {
            std::cerr << "Error: EncIndLoc::writeToFirstEmpty(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }

        bigint itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndLoc::writeToFirstEmpty(): error reading from file "
                      << this->filename << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        positionsSeen++;

        pos = (pos + collisionSkip) % this->size;
        // if either we need to `fseek()` to further than we had `fread()` (i.e. `collisionSkip`
        // > 1), or `pos` had been decreased this iteration (i.e. `pos < collisionSkip`) meaning
        // we must've wrapped around, call `fseek()` to make sure we are on the correct position
        // (otherwise the previous `fread()` automatically handles it, so we can save some time)
        if (collisionSkip > 1 || pos < collisionSkip) {
            std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
        }
    } while (std::memcmp(currEntry, NULL_ENTRY, ENTRY_LEN) != 0);
    this->benchmark->stopProfile("fread2");

    // write into the empty location we found
    this->write(pos, encIndEntry);
}
