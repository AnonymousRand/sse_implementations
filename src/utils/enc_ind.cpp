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
// (technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv pair, but currently `EncInd::ENTRY_LEN` is 1536 bits so there's a 2^1536 chance of this happening...
// and USENIX'24's implementation seems to just do this too)
const uchar EncInd::NULL_ENTRY[EncInd::ENTRY_LEN] = {};


EncInd::~EncInd() {
    this->clear();
}


void EncInd::init(int64_t size) {
    this->clear();
    this->size = size;

    // avoid naming clashes if multiple indexes are active at the same time (e.g. Log-SRC-i, SDa)
    // I spent like four hours trying to debug Log-SRC-i without realizing that its second index was just overwriting
    // the same file its first index was being stored in...
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
        int itemsWritten = std::fwrite(EncInd::NULL_ENTRY, EncInd::ENTRY_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error: EncInd::init(): error initializing file (nothing written)" << std::endl;
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
    uchar currEntry[EncInd::ENTRY_LEN];
    std::fseek(this->file, pos * EncInd::ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(currEntry, EncInd::ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::find(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if entry at `pos` did not match the target `key` (i.e. another kv pair overflowed here first),
    // scan subsequent locations for where the target `key` could've overflowed to
    const uchar* targetKeyCStr = key.c_str();
    int64_t numPositionsChecked = 1;
    while (std::memcmp(currEntry, targetKeyCStr, EncInd::KEY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currEntry, EncInd::ENTRY_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error: EncInd::find(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    // if not found
    if (std::memcmp(currEntry, targetKeyCStr, EncInd::KEY_LEN) != 0) {
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

    uchar entry[EncInd::ENTRY_LEN];
    std::fseek(this->file, pos * EncInd::ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, EncInd::ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::read(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(entry, EncInd::NULL_ENTRY, EncInd::ENTRY_LEN) == 0) {
        // if `pos` contains `NULL_ENTRY`
        return false;
    }

    ret.first = ustring(&entry[EncInd::KEY_LEN], EncInd::DOC_LEN);
    ret.second = ustring(&entry[EncInd::KEY_LEN + EncInd::DOC_LEN], utils::IV_LEN);
    return true;
}


void EncInd::write(uint64_t pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    // check if location at `pos` is already filled
    uchar currEntry[EncInd::ENTRY_LEN];
    std::fseek(this->file, pos * EncInd::ENTRY_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currEntry, EncInd::ENTRY_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "Error: EncInd::write(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
    int64_t numPositionsChecked = 1;
    while (std::memcmp(currEntry, EncInd::NULL_ENTRY, EncInd::ENTRY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currEntry, EncInd::ENTRY_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "Error: EncInd::write(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currEntry, EncInd::NULL_ENTRY, EncInd::ENTRY_LEN) != 0) {
        std::cerr << "Error: EncInd::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // once we've found our spot, perform the write
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring entry = key + val.first + val.second;
    if (entry.length() != EncInd::ENTRY_LEN) {
        std::cerr << "Error: EncInd::write(): write of length " << entry.length() << " bytes is not allowed! "
                  << "(want " << EncInd::ENTRY_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::fseek(this->file, pos * EncInd::ENTRY_LEN, SEEK_SET); // go back to the correct spot, undoing last `fread`
    int itemsWritten = std::fwrite(entry.c_str(), EncInd::ENTRY_LEN, 1, this->file);
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

    uchar entry[EncInd::ENTRY_LEN];
    std::fseek(this->file, pos * EncInd::ENTRY_LEN, SEEK_SET);
    int itemsRead = std::fread(entry, EncInd::ENTRY_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: EncInd::get(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ustring key = ustring(&entry[0], EncInd::KEY_LEN);
    ustring doc = ustring(&entry[EncInd::KEY_LEN], EncInd::DOC_LEN);
    ustring iv = ustring(&entry[EncInd::KEY_LEN + EncInd::DOC_LEN], utils::IV_LEN);
    return EncIndEntry {key, EncIndVal {doc, iv}};
};


void EncInd::print() const {
    for (int64_t pos = 0; pos < this->size; pos++) {
        EncIndEntry entry = this->get(pos);
        std::cerr << pos << ": " << utils::ustrToHex(utils::toUstr(entry)) << std::endl << std::endl;
    }
}
