#include "utils/types/enc_ind/enc_ind_base.h"

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


void EncIndBase::init(bigint capacity) {
    // inits DB file and file pointer
    IDiskStorage::init();

    this->capacity = capacity;

    // fill file with zero bits
    this->benchmark->startProfile("init");
    for (bigint i = 0; i < this->capacity; i++) {
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
    this->capacity = 0;

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(ubigint pos, EncIndVal& ret) const {
    pos %= this->capacity;

    uchar entry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readEncoded(entry);
    if (std::memcmp(entry, NULL_ENTRY, ENTRY_LEN) == 0) {
        // if `pos` contains `NULL_ENTRY`
        return false;
    }

    ret.first = ustring(&entry[KEY_LEN], DATA_LEN);
    ret.second = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(pos, key.c_str(), KEY_LEN);
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry) {
    pos %= this->capacity;

    // encode `encIndEntry` into one string
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring encodedEntry = key + val.first + val.second;
    if (encodedEntry.length() != ENTRY_LEN) {
        std::cerr << "Error: EncIndBase::write(): write of length " << encodedEntry.length()
                  << " bytes is not allowed! (want " << ENTRY_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // then go to `pos` and write the encoded `encIndEntry`
    this->writeEncoded(pos, encodedEntry.c_str());
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


void EncIndBase::readEncoded(uchar* buf) const {
    this->benchmark->startProfile("fflush");
    this->flushIfNotFlushed();
    this->benchmark->stopProfile("fflush");

    this->benchmark->startProfile("fread");
    bigint itemsRead = std::fread(buf, ENTRY_LEN, 1, this->file);
    this->benchmark->stopProfile("fread");
    if (itemsRead != 1) {
        std::cerr << "Error: EncIndBase::readEncoded(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void EncIndBase::writeEncoded(ubigint pos, const uchar* encodedEntry) {
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->benchmark->startProfile("fwrite");
    int itemsWritten = std::fwrite(encodedEntry, ENTRY_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: EncIndBase::writeEncoded(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->benchmark->stopProfile("fwrite");
    this->isFlushed = false;
}


EncIndEntry EncIndBase::getEncIndEntry(ubigint pos) const {
    pos %= this->capacity;

    uchar entry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readEncoded(entry);

    ustring key = ustring(&entry[0], KEY_LEN);
    ustring data = ustring(&entry[KEY_LEN], DATA_LEN);
    ustring iv = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return EncIndEntry {key, EncIndVal {data, iv}};
};


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry entry = this->getEncIndEntry(pos);
        std::cerr << pos << ": " << utils::debugging::ustrToHex(utils::enc_ind::toUstr(entry))
                  << std::endl << std::endl;
    }
}
