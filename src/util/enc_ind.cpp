#include <cstring>

#include "enc_ind.h"

/******************************************************************************/
/* `RamEncInd`                                                                */
/******************************************************************************/

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

void RamEncInd::reset() {
    this->map.clear();
}

/******************************************************************************/
/* `DiskEncInd`                                                               */
/******************************************************************************/

DiskEncInd::~DiskEncInd() {
    this->reset();
}

void DiskEncInd::init(unsigned long size) {
    this->size = size;
    this->buf = new unsigned char[size * ENC_IND_KV_LEN];

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
        std::cerr << "Error opening encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (unsigned long i = 0; i < size; i++) {
        this->isPosFilled[i] = false;
    }

    std::cout << "init called, filename is " << this->filename << " file pointer is " << this->file << std::endl;
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
    // if location is already filled (e.g. because of modulo), find next available location
    unsigned long numPositionsChecked = 1;
    while (this->isPosFilled[pos] && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
    }
    if (this->isPosFilled[pos]) {
        std::cerr << "Ran out of space writing to disk encrypted index!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->isPosFilled[pos] = true;

    // encode kv pair and write to buffer
    ustring kvPair = key + val.first + val.second;
    std::memcpy(&this->buf[pos * ENC_IND_KV_LEN], &kvPair[0], ENC_IND_KV_LEN);
}

void DiskEncInd::flushWrite() {
    std::fwrite(this->buf, ENC_IND_KV_LEN, size, this->file);
    // free up memory that is no longer needed as well
    delete[] this->buf;
    this->buf = nullptr;
    this->isPosFilled.clear();
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
        delete[] curr;
        return -1;
    }

    // decode kv pair and return it
    ret.first = ustring(&curr[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&curr[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
    delete[] curr;
    return 0;
}

void DiskEncInd::reset() {
    std::cout << "reset called for file " << this->filename << std::endl;
    if (this->file != nullptr) {
        std::cout << "file exists, filename is " << this->filename << " file pointer is " << this->file << std::endl;
        std::fclose(this->file);
        this->file = nullptr; // important for idempotence!
    }
    if (this->buf != nullptr) {
        delete[] this->buf;
        this->buf = nullptr;
    }
    // delete encrypted index files from disk on `reset()`
    if (this->filename != "") {
        std::cout << "file removed " << this->filename << std::endl;
        std::remove(this->filename.c_str());
        this->filename = "";
    } else {
        std::cout << "no remove!!!" << std::endl;
    }
    this->isPosFilled.clear();
}
