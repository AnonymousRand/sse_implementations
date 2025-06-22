#include <cstring>

#include "enc_ind.h"

////////////////////////////////////////////////////////////////////////////////
// `RamEncInd`
////////////////////////////////////////////////////////////////////////////////

void RamEncInd::init(unsigned long size) {}

void RamEncInd::write(ustring key, std::pair<ustring, ustring> val) {
    this->map[key] = val;
}

void RamEncInd::flushWrite() {}

int RamEncInd::find(ustring key, std::pair<ustring, ustring>& ret) const {
    auto iter = this->map.find(key);
    if (iter == this->map.end()) {
        return -1;
    }
    ret = iter->second;
    return 0;
}

void RamEncInd::clear() {
    this->map.clear();
}

////////////////////////////////////////////////////////////////////////////////
// `DiskEncInd`
////////////////////////////////////////////////////////////////////////////////

DiskEncInd::DiskEncInd() : buf(nullptr), file(nullptr) {}

DiskEncInd::~DiskEncInd() {
    this->clear();
}

void DiskEncInd::init(unsigned long size) {
    this->size = size;
    this->buf = new unsigned char[size * ENC_IND_KV_LEN](); // `()` fills buffer with '\0' bits
    // avoid naming clashes if multiple indexes are active at the same time (e.g. Log-SRC-i, SDa)
    // I spent like four hours trying to debug Log-SRC-i without realizing that its second index was just overwriting
    // the same file its first index was being stored in...
    std::uniform_int_distribution dist(100000000, 999999999);
    std::string filename = "out/enc_ind_" + std::to_string(dist(RNG)) + ".dat";
    FILE* fileTmp = std::fopen(filename.c_str(), "r");
    // while file exists (or any other error occurs on open), create new random filename
    while (fileTmp != nullptr) {
        std::fclose(fileTmp);
        filename = "out/enc_ind_" + std::to_string(dist(RNG)) + ".dat";
        fileTmp = std::fopen(filename.c_str(), "r");
    }

    this->file = std::fopen(filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error opening encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

/**
 * PRECONDITION:
 *     - `key` must be exactly `ENC_IND_KEY_LEN` long
 *     - `val.first` must be exactly `ENC_IND_DOC_LEN` long (e.g. via `padAndEncrypt()`)
 *     - `val.second` (the IV) must be exactly `IV_LEN` long
 */
void DiskEncInd::write(ustring key, std::pair<ustring, ustring> val) {
    // try to place encrypted items in the location specified by its encrypted key, i.e. PRF/hash output for PiBas 
    // (modulo buffer size); this seems iffy because of modulo but this is what USENIX'24's implementation does
    // (although they also use caching, presumably since it's slow if we need to keep finding next available locations)
    // this conversion mess is from USENIX'24's implementation
    unsigned long pos = (*((unsigned long*)key.c_str())) % this->size;
    // if location is already filled (e.g. because of modulo), find next available location (which should contain '\0')
    unsigned long numPositionsChecked = 1;
    while (this->buf[pos * ENC_IND_KV_LEN] != '\0' && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
    }
    if (this->buf[pos * ENC_IND_KV_LEN] != '\0') {
        std::cerr << "Ran out of space writing to disk encrypted index!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // encode kv pair and write to buffer
    ustring kvPair = key + val.first + val.second;
    std::memcpy(&this->buf[pos * ENC_IND_KV_LEN], &kvPair[0], ENC_IND_KV_LEN);
}

void DiskEncInd::flushWrite() {
    std::fwrite(this->buf, ENC_IND_KV_LEN, size, this->file);
    delete[] this->buf;
    this->buf = nullptr;
}

int DiskEncInd::find(ustring key, std::pair<ustring, ustring>& ret) const {
    unsigned long pos = (*((unsigned long*)key.c_str())) % this->size;
    unsigned char* curr = new unsigned char[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    std::fread(curr, ENC_IND_KV_LEN, 1, this->file);
    ustring currKey(curr, ENC_IND_KEY_LEN);
    // if location based on `key` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    unsigned long numPositionsChecked = 1;
    while (currKey != key && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        // by assuming previous `fread()` read all `ENC_IND_KV_LEN` bytes and hence only needing to `fseek()` when we
        // wrap around to position 0, we make searches about twice as fast
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        std::fread(curr, ENC_IND_KV_LEN, 1, this->file);
        currKey = ustring(curr, ENC_IND_KEY_LEN);
    }
    // this does make it a lot slower to verify if an element is nonexistent compared to primary memory storage,
    // since we have to iterate through whole index
    if (currKey != key) {
        return -1;
    }

    // decode kv pair and return it
    ret.first = ustring(&curr[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&curr[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
    delete[] curr;
    return 0;
}

void DiskEncInd::clear() {
    if (this->buf != nullptr) {
        delete[] this->buf;
        this->buf = nullptr; // important for idempotence!
    }
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }
}
