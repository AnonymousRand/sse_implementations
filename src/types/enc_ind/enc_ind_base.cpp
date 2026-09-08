#include "types/enc_ind/enc_ind_base.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "types/basic_types.h"
#include "types/i_disk_storage.h"
#include "types/ustring.h"

#include "utils/benchmark.h"
#include "utils/debug.h"


//------------------------------------------------------------------------------
// constructors/destructors


EncIndBase::~EncIndBase() {
    this->clear();
}


//------------------------------------------------------------------------------
// rule of five


// copy constructor
EncIndBase::EncIndBase(const EncIndBase& other) {
    IDiskStorage::copyFrom(other);
}


//------------------------------------------------------------------------------
// interface


void EncIndBase::init(bigint capacity) {
    // inits DB file and file pointer
    IDiskStorage::init();

    // this initializes `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()()` relies on virtual methods
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


bool EncIndBase::read(ubigint pos, EncIndVal& ret) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    utils::benchmark::startProfile("fseek");
    std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    utils::benchmark::stopProfile("fseek");
    this->readEncoded(entry);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret.first = ustring(&entry[this->KEY_LEN()], this->DATA_LEN());
    ret.second = ustring(&entry[this->KEY_LEN() + this->DATA_LEN()], utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(pos, key.c_str(), this->KEY_LEN());
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
    DEBUG_ONLY({
        if (encodedEntry.length() != this->ENTRY_LEN()) {
            std::cerr << "Error: EncIndBase::write(): write of length " << encodedEntry.length()
                      << " bytes is not allowed! (want " << this->ENTRY_LEN() << " bytes)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // then go to `pos` and write the encoded `encIndEntry`
    this->writeEncoded(pos, encodedEntry.c_str());
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(pos, this->NULL_ENTRY, this->ENTRY_LEN());
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
        this->readEntry(pos, encIndEntry);
        std::cerr << pos << ": " << utils::debug::ustrToHex(utils::enc_ind::toUstr(encIndEntry))
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


bool EncIndBase::readEntry(ubigint pos, EncIndEntry& ret) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    utils::benchmark::startProfile("fseek");
    std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    utils::benchmark::stopProfile("fseek");
    this->readEncoded(entry);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ustring key(&entry[0], this->KEY_LEN());
    ustring data(&entry[this->KEY_LEN()], this->DATA_LEN());
    ustring iv(&entry[this->KEY_LEN() + this->DATA_LEN()], utils::crypto::IV_LEN);
    ret = EncIndEntry {key, EncIndVal {data, iv}};
    return true;
};


void EncIndBase::readEncoded(uchar* buf) const {
    utils::benchmark::startProfile("fflush");
    this->flushIfNotFlushed();
    utils::benchmark::stopProfile("fflush");

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


void EncIndBase::writeEncoded(ubigint pos, const uchar* encodedEntry) {
    utils::benchmark::startProfile("fseek");
    std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    utils::benchmark::stopProfile("fseek");
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
}
