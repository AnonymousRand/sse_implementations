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
    //std::cout << "----- writing to pos " << pos << std::endl;
    //std::cout << "key is " << utils::debugging::ustrToHex(entry, KEY_LEN) << std::endl;
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
        //std::cout << "----- finding, pos is " << pos << ", positions checked is " << positionsChecked << ", collision attempts is " << collisionAttempts << std::endl;
        //std::cout << "key is " << utils::debugging::ustrToHex(currEntry, KEY_LEN) << std::endl;
        // if still not found after `collisionAttempts` iterations forward, give up
        if (positionsChecked == collisionAttempts) {
            //std::cout << "----- DIDN'T FIND!" << std::endl;
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
    //std::cout << "----- FOUND!! at pos " << pos << std::endl;;

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::writeToFirstEmptyBase(
    ubigint& pos, const EncIndEntry& encIndEntry, bigint collisionSkip, bigint collisionAttempts
) {
    pos %= this->size;

    this->benchmark->startProfile("fread2");
    // first check if location at `pos` is already filled (e.g. because of `pos %= this->size`)
    // if it is, find the next available location, iterating forward by `collisionSkip` positions
    // at a time (this is also what USENIX'24's implementation does)
    // >>TODO if this works out, do similar optimizations for `findBase()`? or no since that is
    // where locality shines?
    // TODO: add fast setup config option and this buffer size
    constexpr int READ_BUF_TARGET_ENTRIES = 512;
    // (note that this isi always guaranteed to be small enough to be an `int`)
    const int readBufEntryCount = std::min((bigint)READ_BUF_TARGET_ENTRIES, this->size);
    uchar readBuf[readBufEntryCount * ENTRY_LEN];
    int readBufIndex = 0;
    bigint positionsChecked = 0;
    //std::cout << "+++++ ATTEMPTING to write to pos " << pos << "; size is " << this->size << std::endl;
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->readIntoReadBuf(readBuf, readBufEntryCount);
    while (std::memcmp(readBuf + (readBufIndex * ENTRY_LEN), NULL_ENTRY, ENTRY_LEN) != 0)
    {
        positionsChecked++;
        // if we've done `collisionAttempts` iterations and still haven't found an available space,
        // throw an error (this usually means we are trying to write to a full index)
        if (positionsChecked == collisionAttempts) {
            std::cerr << "Error: EncIndBase::writeToFirstEmptyBase(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }
        //std::cout << "positions seen: " << positionsChecked << "; pos: " << pos << "readBufIndex: " << readBufIndex << "; collisionSkip: " << collisionSkip << "; collisionAttempts: " << collisionAttempts << std::endl;

        // this should be the only place we handle updating `pos`
        readBufIndex += collisionSkip;
        pos = (pos + collisionSkip) % this->size;

        // if we've gotten to the end of the current `readBuf`, read the next part of the file
        // into it, and also reset its internal `readBufIndex` index
        if (readBufIndex >= readBufEntryCount) {
            // (note: the file pointer should be in the right position from the last `fread()`,
            // assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
            this->readIntoReadBuf(readBuf, readBufEntryCount);
            readBufIndex = 0;
        }
        //std::cout << "about to check index " << readBufIndex << std::endl;
    }
    this->benchmark->stopProfile("fread2");

    // write into the empty location we found
    this->write(pos, encIndEntry);
}


void EncIndBase::readIntoReadBuf(uchar* readBuf, int readBufEntryCount) const {
    int itemsRead = std::fread(readBuf, ENTRY_LEN, readBufEntryCount, this->file);
    //std::cout << "read " << itemsRead << " into buffer" << std::endl;
    // wrap around to beginning of file if we read less than the target number of entries
    if (itemsRead < readBufEntryCount) {
        std::fseek(this->file, 0, SEEK_SET);
        itemsRead += std::fread(
            readBuf + (itemsRead * ENTRY_LEN), ENTRY_LEN, readBufEntryCount - itemsRead, this->file
        );
        //std::cout << "read " << itemsRead << " more items into buffer" << std::endl;
        if (itemsRead < readBufEntryCount) {
            std::cerr << "Error: EncIndBase::writeToFirstEmptyBase(): error reading "
                      << " from file " << this->filename
                      << " (only read " << itemsRead << " out of " << readBufEntryCount
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
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
