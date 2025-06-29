#include <cstring>
#include <iomanip>

#include "enc_ind.h"


// for debugging
void printStrAsHex(const unsigned char* str, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(str[i]) << " ";
    }
    std::cout << std::endl;
}


/******************************************************************************/
/* `EncIndRam`                                                                */
/******************************************************************************/


void EncIndRam::init(unsigned long size) {}


void EncIndRam::write(ustring label, std::pair<ustring, ustring> val) {
    this->map[label] = val;
}


int EncIndRam::find(ustring label, std::pair<ustring, ustring>& ret) const {
    auto iter = this->map.find(label);
    if (iter == this->map.end()) {
        return -1;
    }
    ret = iter->second;
    return 0;
}


void EncIndRam::clear() {
    this->map.clear();
}


/******************************************************************************/
/* `EncIndDisk`                                                               */
/******************************************************************************/


// this initializes everything to `\0`, i.e. zero bits
// technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv-pair, but currently `ENC_IND_KV_LEN` is 1024 bits so there's a 2^1024 chance of this happening
// USENIX'24's implementation also seems to just do this
const unsigned char EncIndDisk::nullKv[] = {};


EncIndDisk::~EncIndDisk() {
    this->clear();
}


void EncIndDisk::init(unsigned long size) {
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
        std::cerr << "Error opening encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // fill file with zero bits
    for (unsigned long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(EncIndDisk::nullKv, ENC_IND_KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error initializing encrypted index file: wrote no bytes" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


/**
 * PRECONDITION:
 *     - `key` must be exactly `ENC_IND_KEY_LEN` long
 *     - `val.first` must be exactly `ENC_IND_DOC_LEN` long (e.g. via `padAndEncrypt()`)
 *     - `val.second` (the IV) must be exactly `IV_LEN` long
 */
void EncIndDisk::write(ustring label, std::pair<ustring, ustring> val) {
    // try to place encrypted items in the location specified by its encrypted label, i.e. PRF/hash output for PiBas 
    // (modulo buffer size); this seems iffy because of modulo but this is what USENIX'24's implementation does
    // (although they also use caching, presumably since it's slow if we need to keep finding next available locations)
    // this conversion mess is from USENIX'24's implementation
    unsigned long pos = (*((unsigned long*)label.c_str())) % this->size;
    unsigned char* currKv = new unsigned char[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error reading encrypted index file on `write()`: read no bytes" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (e.g. because of modulo), find next available location
    unsigned long numPositionsChecked = 1;
    while (std::strcmp((char*)currKv, (char*)EncIndDisk::nullKv) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file); 
        if (itemsRead != 1) {
            std::cerr << "Error reading encrypted index file on `write()`: read no bytes" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::strcmp((char*)currKv, (char*)EncIndDisk::nullKv) != 0) {
        std::cerr << "Ran out of space writing to disk encrypted index!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // encode kv pair and write to file
    ustring kvPair = label + val.first + val.second;
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    std::fwrite(kvPair.c_str(), ENC_IND_KV_LEN, 1, this->file);
    std::fflush(this->file); // flush to guarantee that we immediately mark the spot we just wrote into as filled
    delete[] currKv;
    currKv = nullptr;
}


int EncIndDisk::find(ustring label, std::pair<ustring, ustring>& ret) const {
    unsigned long pos = (*((unsigned long*)label.c_str())) % this->size;
    unsigned char* currKv = new unsigned char[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error reading encrypted index file on `find()`: read no bytes" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ustring currLabel(currKv, ENC_IND_KEY_LEN);

    // if location based on `label` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    unsigned long numPositionsChecked = 1;
    while (currLabel != label && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error reading encrypted index file on `find()`: read no bytes" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        currLabel = ustring(currKv, ENC_IND_KEY_LEN);
    }
    // this does make it a lot slower to verify if an element is nonexistent compared to primary memory storage,
    // since we have to iterate through whole index
    // but this only makes reducing false positives all the more important
    if (currLabel != label) {
        delete[] currKv;
        currKv = nullptr;
        return -1;
    }

    // decode kv pair and return it
    ret.first = ustring(&currKv[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&currKv[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
    delete[] currKv;
    currKv = nullptr;
    return 0;
}


void EncIndDisk::clear() {
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr; // important for idempotence!
    }
    // delete encrypted index files from disk on `clear()`
    if (this->filename != "") {
        std::remove(this->filename.c_str());
        this->filename = "";
    }
}
