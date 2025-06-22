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
    this->buf = new unsigned char[size * ENC_IND_KV_LEN](); // `()` fills buffer with '\0' bits
    this->file = std::fopen("enc_ind.dat", "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error opening encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->size = size;
}

/**
 * PRECONDITION:
 *     - `key` must be exactly `ENC_IND_KEY_LEN` long
 *     - `val` must be exactly `ENC_IND_VAL_LEN` long (e.g. via `padAndEncrypt()`)
 *     - `val.second` (the IV) must be exactly `IV_LEN` long
 */
void DiskEncInd::write(ustring key, std::pair<ustring, ustring> val) {
    // try to place encrypted items in the location specified by its encrypted key, i.e. PRF/hash output for PiBas 
    // (modulo bufay size); this seems iffy because of modulo but this is what USENIX'24's implementation does
    // (although they also use caching, presumably since it's slow if we need to keep finding next available locations)
    unsigned long pos = std::stoul(fromUstr(key)) % this->size;
    // if location is already filled (e.g. because of modulo), find next available location (which should contain '\0')
    unsigned long counter = 1;
    while (this->buf[pos * ENC_IND_KV_LEN] != '\0' && counter <= this->size) {
        counter++;
        pos = (pos + 1) % this->size;
    }
    if (counter >= this->size) {
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
    unsigned long pos = std::stoul(fromUstr(key)) % this->size;
    unsigned char* curr = new unsigned char[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    std::fread(curr, ENC_IND_KV_LEN, 1, this->file);
    ustring currKey(curr, ENC_IND_KEY_LEN);
    // if location based on `key` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    unsigned long counter = 1;
    while (currKey != key && counter <= this->size) {
        counter++;
        pos = (pos + 1) % this->size;
        std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
        std::fread(curr, ENC_IND_KV_LEN, 1, this->file);
        currKey = ustring(curr, ENC_IND_KEY_LEN);
    }
    // TODO this is so slow though to detect nonexistent elements (which is required by design)
    if (counter >= this->size) {
        return -1;
    }

    // decode kv pair and return it
    ret.first = ustring(&curr[ENC_IND_KEY_LEN], ENC_IND_VAL_LEN - IV_LEN);
    ret.second = ustring(&curr[ENC_IND_KV_LEN - IV_LEN], IV_LEN);
    delete[] curr;
    return 0;
}

void DiskEncInd::clear() {
    if (this->buf != nullptr) {
        delete[] this->buf;
    }
    if (this->file != nullptr) {
        std::fclose(this->file);
    }
}
