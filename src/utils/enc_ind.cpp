#include "utils/enc_ind.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>

#include "utils/debugging.h"
#include "utils/random.h"
#include "utils/ustring.h"


//==============================================================================
// utils
//==============================================================================


namespace utils {


ustring toUstr(const EncIndEntry& encIndEntry) {
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    return key + val.first + val.second;
}


} // namespace `utils`


//==============================================================================
// `EncInd`
//==============================================================================


// this initializes `NULL_ENTRY` to a contiguous block of zero bits
// (technically it is possible that some encrypted tuple happened to be all `0` bytes
// and thus get mistaken for a null kv pair, but currently `ENTRY_LEN` is in the
// thousands of bits so there's a 2^{>1000} chance of this happening...and USENIX'24's
// implementation seems to just do this too)
const uchar EncInd::NULL_ENTRY[ENTRY_LEN] = {};


EncInd::~EncInd() {
    this->clear();
}


void EncInd::init(int64_t size) {
    this->clear();
    this->size = size;

    // avoid naming clashes if multiple indexes are active at the same time (e.g. Log-SRC-i, SDa)
    // I spent like four hours trying to debug Log-SRC-i without realizing that its second index
    // was just overwriting the same file its first index was being stored in...
    std::uniform_int_distribution dist(100000000, 999999999);
    this->filename = "out/enc_ind_" + std::to_string(dist(utils::RNG)) + ".dat";
    FILE* fileTmp = std::fopen(this->filename.c_str(), "r");
    // while file exists (or any other error occurs on open), create new random filename
    while (fileTmp != nullptr) {
        std::fclose(fileTmp);
        this->filename = "out/enc_ind_" + std::to_string(dist(utils::RNG)) + ".dat";
        fileTmp = std::fopen(this->filename.c_str(), "r");
    }
    this->file = std::fopen(this->filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error: EncInd::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // fill file with zero bits
    for (int64_t i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(NULL_ENTRY, ENTRY_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error: EncInd::init(): error initializing file (nothing written)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


void EncInd::clear() {
    // close encrypted index file descriptors
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }
    // delete encrypted index files
    if (this->filename != "") {
        std::remove(this->filename.c_str());
        this->filename = "";
    }
    this->size = 0;
}


bool EncInd::find(uint64_t pos, const ustring& key, EncIndVal& ret, uint64_t* posFoundAt) const {
    pos %= this->size;

    // get entry at `pos`
    uchar currTuple[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currTuple, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::find(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if entry at `pos` did not match the target `key` (i.e. another kv pair overflowed here
    // first), scan subsequent locations for where the target `key` could've overflowed to
    const uchar* targetKeyCStr = key.c_str();
    int64_t numPositionsChecked = 1;
    while (std::memcmp(currTuple, targetKeyCStr, KEY_LEN) != 0
           && numPositionsChecked < this->size)
    {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currTuple, ENTRY_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error: EncInd::find(): error reading from file (nothing read)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    // if not found
    if (std::memcmp(currTuple, targetKeyCStr, KEY_LEN) != 0) {
        return false;
    }

    // decode kv pair and return it
    if (posFoundAt != nullptr) {
        *posFoundAt = pos;
    }
    return this->read(pos, ret);
}


bool EncInd::read(uint64_t pos, EncIndVal& ret) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::read(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(entry, NULL_ENTRY, ENTRY_LEN) == 0) {
        // if `pos` contains `NULL_ENTRY`
        return false;
    }

    ret.first = ustring(&entry[KEY_LEN], DATA_LEN);
    ret.second = ustring(&entry[KEY_LEN + DATA_LEN], crypto::IV_LEN);
    return true;
}


void EncInd::write(uint64_t pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    // check if location at `pos` is already filled
    uchar currTuple[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currTuple, ENTRY_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "Error: EncInd::write(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
    // (this is what USENIX'24's implementation does)
    int64_t numPositionsChecked = 1;
    while (std::memcmp(currTuple, NULL_ENTRY, ENTRY_LEN) != 0
           && numPositionsChecked < this->size)
    {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currTuple, ENTRY_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "Error: EncInd::write(): error reading from file (nothing read)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currTuple, NULL_ENTRY, ENTRY_LEN) != 0) {
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
        std::cerr << "Error: EncInd::write(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // flush immediately to mark space as occupied
    std::fflush(this->file);
}


int64_t EncInd::getSize() const {
    return this->size;
}


//------------------------------------------------------------------------------
// debugging


EncIndEntry EncInd::get(uint64_t pos) const {
    pos %= this->size;

    uchar entry[ENTRY_LEN];
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::get(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ustring key = ustring(&entry[0], KEY_LEN);
    ustring tuple = ustring(&entry[KEY_LEN], DATA_LEN);
    ustring iv = ustring(&entry[KEY_LEN + DATA_LEN], crypto::IV_LEN);
    return EncIndEntry {key, EncIndVal {tuple, iv}};
};


void EncInd::print() const {
    for (int64_t pos = 0; pos < this->size; pos++) {
        EncIndEntry entry = this->get(pos);
        std::cerr << pos << ": " << utils::ustrToHex(utils::toUstr(entry))
                  << std::endl << std::endl;
    }
}
