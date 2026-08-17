#include "utils/types/enc_ind.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

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
// `EncInd`
//==============================================================================


// this initializes `NULL_ENTRY` to a contiguous block of zero bits
// (technically it is possible that some encrypted tuple happened to be all `0` bytes
// and thus get mistaken for a null kv pair, but currently `ENTRY_LEN` is in the
// thousands of bits so there's a 2^{>1000} chance of this happening...and USENIX'24's
// implementation seems to just do this too)
const uchar EncInd::NULL_ENTRY[ENTRY_LEN] = {};


//------------------------------------------------------------------------------
// copy/move


// copy constructor
EncInd::EncInd(const EncInd& other) {
    IDiskStorage::copyFrom(other);
}


//------------------------------------------------------------------------------
// `IDiskStorage`


void EncInd::init(bigint size) {
    // inits DB file and file pointer
    IDiskStorage::init();
    this->size = size;

    // fill file with zero bits
    for (bigint i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(NULL_ENTRY, ENTRY_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error: EncInd::init(): error initializing file (nothing written)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    std::fflush(this->file);
}


void EncInd::clear() {
    // clears DB file and file pointer
    IDiskStorage::clear();

    this->size = 0;
}


//------------------------------------------------------------------------------
// other interface


bool EncInd::find(ubigint pos, const ustring& key, EncIndVal& ret, ubigint* posFoundAt) const {
    pos %= this->size;

    // get entry at `pos`
    uchar currEntry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::find(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if entry at `pos` did not match the target `key` (i.e. another kv pair overflowed here
    // first), scan subsequent locations for where the target `key` could've overflowed to
    const uchar* targetKeyCStr = key.c_str();
    bigint numPositionsChecked = 1;
    while (std::memcmp(currEntry, targetKeyCStr, KEY_LEN) != 0
           && numPositionsChecked < this->size)
    {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error: EncInd::find(): error reading from file " << this->filename
                      << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    // if not found
    if (std::memcmp(currEntry, targetKeyCStr, KEY_LEN) != 0) {
        return false;
    }

    // decode kv pair and return it
    if (posFoundAt != nullptr) {
        *posFoundAt = pos;
    }
    return this->read(pos, ret);
}


bool EncInd::read(ubigint pos, EncIndVal& ret) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::read(): error reading from file " << this->filename
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


void EncInd::write(ubigint pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    // check if location at `pos` is already filled
    uchar currEntry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::write(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
    // (this is what USENIX'24's implementation does)
    bigint numPositionsChecked = 1;
    while (std::memcmp(currEntry, NULL_ENTRY, ENTRY_LEN) != 0
           && numPositionsChecked < this->size)
    {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currEntry, ENTRY_LEN, 1, this->file); 
        if (itemsRead != 1) {
            std::cerr << "Error: EncInd::write(): error reading from file " << this->filename
                      << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currEntry, NULL_ENTRY, ENTRY_LEN) != 0) {
        std::cerr << "Error: EncInd::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // once we've found our spot, perform the write
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring entry = key + val.first + val.second;
    if (entry.length() != ENTRY_LEN) {
        std::cerr << "Error: EncInd::write(): write of length " << entry.length()
                  << " bytes is not allowed! "
                  << "(want " << ENTRY_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // go back to the correct spot, undoing last `fread`
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsWritten = std::fwrite(entry.c_str(), ENTRY_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: EncInd::write(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // flush immediately to mark space as occupied (and so we always have `this->isFlushed = true`)
    std::fflush(this->file);
}


bigint EncInd::getSize() const {
    return this->size;
}


//------------------------------------------------------------------------------
// debugging


EncIndEntry EncInd::get(ubigint pos) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::get(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ustring key = ustring(&entry[0], KEY_LEN);
    ustring data = ustring(&entry[KEY_LEN], DATA_LEN);
    ustring iv = ustring(&entry[KEY_LEN + DATA_LEN], utils::crypto::IV_LEN);
    return EncIndEntry {key, EncIndVal {data, iv}};
};


void EncInd::print() const {
    for (bigint pos = 0; pos < this->size; pos++) {
        EncIndEntry entry = this->get(pos);
        std::cerr << pos << ": " << utils::debugging::ustrToHex(utils::enc_ind::toUstr(entry))
                  << std::endl << std::endl;
    }
}
