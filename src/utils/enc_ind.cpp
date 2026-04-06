#include "enc_ind.h"


// this initializes everything to `\0`, i.e. zero bits
// technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv-pair, but currently `EncInd::KV_LEN` is 1024 bits so there's a 2^1024 chance of this happening
// USENIX'24's implementation also seems to just do this
const uchar EncInd::NULL_KV[EncInd::KV_LEN] = {};


EncInd::~EncInd() {
    this->clear();
}


void EncInd::init(long size) {
    this->clear();
    this->size = size;

    // avoid naming clashes if multiple indexes are active at the same time (e.g. Log-SRC-i, SDa)
    // I spent like four hours trying to debug Log-SRC-i without realizing that its second index was just overwriting
    // the same file its first index was being stored in...
    std::uniform_int_distribution dist(100000000, 999999999);
    this->filename = "out/enc_ind_" + std::to_string(dist(RNG)) + ".dat";
    FILE* fileTmp = std::fopen(this->filename.c_str(), "r");
    // while file exists (or any other error occurs on open), create new random filename
    while (fileTmp != nullptr) {
        std::fclose(fileTmp);
        this->filename = "out/enc_ind_" + std::to_string(dist(RNG)) + ".dat";
        fileTmp = std::fopen(this->filename.c_str(), "r");
    }
    this->file = std::fopen(this->filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "EncInd::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // fill file with zero bits
    for (long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(EncInd::NULL_KV, EncInd::KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "EncInd::init(): error initializing file (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


void EncInd::clear() {
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }
    // delete encrypted index files from disk on `clear()`
    if (this->filename != "") {
        std::remove(this->filename.c_str());
        this->filename = "";
    }
    this->size = 0;
}


bool EncInd::find(ulong pos, const ustring& key, EncIndVal& ret) const {
    pos %= this->size;

    uchar currKv[EncInd::KV_LEN];
    std::fseek(this->file, pos * EncInd::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, EncInd::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncInd::find(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if entry at `pos` did not match the target `key` (i.e. another kv pair overflowed here first),
    // scan subsequent locations for where the target `key` could've overflowed to
    const uchar* targetKeyCStr = key.c_str();
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, targetKeyCStr, EncInd::KEY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, EncInd::KV_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "EncInd::find(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    // if not found
    if (std::memcmp(currKv, targetKeyCStr, EncInd::KEY_LEN) != 0) {
        return false;
    }

    // decode kv pair and return it
    return this->read(pos, ret);
}


bool EncInd::read(ulong pos, EncIndVal& ret) const {
    pos %= this->size;

    uchar kv[EncInd::KV_LEN];
    std::fseek(this->file, pos * EncInd::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(kv, EncInd::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncInd::read(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(kv, EncInd::NULL_KV, EncInd::KV_LEN) == 0) {
        // if `pos` contains `NULL_KV`
        return false;
    }

    ret.first = ustring(&kv[EncInd::KEY_LEN], EncInd::DOC_LEN);
    ret.second = ustring(&kv[EncInd::KEY_LEN + EncInd::DOC_LEN], IV_LEN);
    return true;
}


void EncInd::write(ulong pos, const EncIndEntry& encIndEntry) {
    pos %= this->size;

    /*
    if (pos >= this->size) {
        std::cerr << "EncInd::write(): write to pos " << pos << " is out of bounds! "
                  << "(size is " << this->size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    */

    // if location is already filled (because of modulo), find next available location
    uchar currKv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncInd::KV_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "EncInd::write(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    long numPositionsChecked = 1;
    while (std::memcmp(currKv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "EncInd::write(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) != 0) {
        std::cerr << "EncInd::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // once we've found our spot, perform the write
    ustring key = encIndEntry.first;
    EncIndVal val = encIndEntry.second;
    ustring kv = key + val.first + val.second;
    if (kv.length() != EncInd::KV_LEN) {
        std::cerr << "EncInd::write(): write of length " << kv.length() << " bytes is not allowed! "
                  << "(want " << EncInd::KV_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    int itemsWritten = std::fwrite(kv.c_str(), EncInd::KV_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "EncInd::write(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // flush immediately to mark space as occupied
    std::fflush(this->file);
}
