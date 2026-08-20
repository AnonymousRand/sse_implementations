#include "utils/types/enc_ind.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
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


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry, Benchmark* benchmark) {
    pos %= this->size;

    // encode `encIndEntry` into one string
    benchmark->startProfile("encode");
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring entry = key + val.first + val.second;
    if (entry.length() != ENTRY_LEN) {
        std::cerr << "Error: EncIndBase::write(): write of length " << entry.length()
                  << " bytes is not allowed! (want " << ENTRY_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    benchmark->stopProfile("encode");

    // then go to `pos` and write the encoded `encIndEntry`
    benchmark->startProfile("fwrite");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsWritten = std::fwrite(entry.c_str(), ENTRY_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: EncIndBase::write(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    benchmark->stopProfile("fwrite");
    // flush immediately to mark space as occupied (and so we always have `this->isFlushed = true`)
    benchmark->startProfile("fflush");
    std::fflush(this->file);
    benchmark->stopProfile("fflush");
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
    const uchar* targetKeyCStr = key.c_str();
    bigint positionsChecked = 1;
    while (positionsChecked < collisionAttempts
           && std::memcmp(currEntry, targetKeyCStr, KEY_LEN) != 0)
    {
        positionsChecked++;
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

    // if still not found after `collisionAttempts` iterations forward, give up
    if (std::memcmp(currEntry, targetKeyCStr, KEY_LEN) != 0) {
        return false;
    }

    // otherwise, now read and decode the kv pair at this matched location
    return this->read(pos, ret);
}


void EncIndBase::writeToFirstEmptyBase(
    ubigint& pos, const EncIndEntry& encIndEntry, Benchmark* benchmark,
    bigint collisionSkip, bigint collisionAttempts
) {
    pos %= this->size;

    // first check if location at `pos` is already filled
    uchar currEntry[ENTRY_LEN];
    benchmark->startProfile("fread");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::writeToFirstEmptyBase(): error reading from file "
                  << this->filename << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    benchmark->stopProfile("fread");

    // if location is already filled (because of modulo), find next available location, iterating
    // forward by `collisionSkip` positions at a time (this is what USENIX'24's implementation does)
    benchmark->startProfile("fread2");
    bigint positionsChecked = 1;
    while (positionsChecked < collisionAttempts
           && std::memcmp(currEntry, NULL_ENTRY, KEY_LEN) != 0)
    {
        positionsChecked++;
        pos = (pos + collisionSkip) % this->size;
        // same optimization as in `findbase()`
        if (collisionSkip != 1) {
            std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
        } else {
            if (pos == 0) {
                std::fseek(this->file, 0, SEEK_SET);
            }
        }
        itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file); 
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndBase::writeToFirstEmptyBase(): error reading from file "
                      << this->filename << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // if still no empty space after `collisionAttempts` iterations forward, index is full so error
    if (std::memcmp(currEntry, NULL_ENTRY, ENTRY_LEN) != 0) {
        std::cerr << "Error: EncIndBase::writeToFirstEmptyBase(): ran out of space writing to "
                  << this->filename << std::endl;
        std::exit(EXIT_FAILURE);
    }
    benchmark->stopProfile("fread2");

    // otherwise, now write into this empty location
    this->write(pos, encIndEntry, benchmark);
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
