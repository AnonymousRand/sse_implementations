#include <cstring>
#include <iomanip>
#include <sstream>

#include "enc_ind.h"


// for debugging
std::string strToHex(const uchar* str, int len) {
    std::stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(str[i]) << " ";
    }
    return ss.str();
}


std::string strToHex(const ustring& str) {
    return strToHex(str.c_str(), str.length());
}


/******************************************************************************/
/* `EncIndRam`                                                                */
/******************************************************************************/


void EncIndRam::init(long size) {
    this->clear();
}


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
const uchar EncIndDisk::nullKv[] = {};


EncIndDisk::~EncIndDisk() {
    this->clear();
}


void EncIndDisk::init(long size) {
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
        std::cerr << "Error opening encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // fill file with zero bits
    for (long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(EncIndDisk::nullKv, ENC_IND_KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "Error initializing encrypted index file: nothing written" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


/**
 * Precondition:
 *     - `key` must be exactly `ENC_IND_KEY_LEN` long
 *     - `val.first` must be exactly `ENC_IND_DOC_LEN` long (e.g. via `padAndEncrypt()`)
 *     - `val.second` (the IV) must be exactly `IV_LEN` long
 */
void EncIndDisk::write(ustring label, std::pair<ustring, ustring> val) {
    // try to place encrypted items in the location specified by its encrypted label, i.e. PRF/hash output for PiBas 
    // (modulo buffer size); this seems iffy because of modulo but this is what USENIX'24's implementation does
    // (although they also use caching, presumably since it's slow if we need to keep finding next available locations)
    // this conversion mess is from USENIX'24's implementation
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currKv[ENC_IND_KV_LEN]; // I don't think we need a null terminator...?
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "Error reading encrypted index file on `write()`: nothing read" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, EncIndDisk::nullKv, ENC_IND_KV_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "Error reading encrypted index file on `write()`: nothing read" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, EncIndDisk::nullKv, ENC_IND_KV_LEN) != 0) {
        std::cerr << "Ran out of space writing to disk encrypted index!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // encode kv pair and write to file
    ustring kv = label + val.first + val.second;
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    itemsReadOrWritten = std::fwrite(kv.c_str(), ENC_IND_KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "Error writing to encrypted index file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::fflush(this->file); // flush to guarantee that we immediately mark the spot we just wrote into as filled
}


int EncIndDisk::find(ustring label, std::pair<ustring, ustring>& ret) const {
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currKv[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error reading encrypted index file on `find()`: nothing read" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ustring currLabel(currKv, ENC_IND_KEY_LEN);

    // if location based on `label` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    long numPositionsChecked = 1;
    while (currLabel != label && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "Error reading encrypted index file on `find()`: nothing read" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        currLabel = ustring(currKv, ENC_IND_KEY_LEN);
    }
    // this does make it a lot slower to verify if an element is nonexistent compared to primary memory storage,
    // since we have to iterate through whole index
    // but this only makes reducing false positives all the more important
    if (currLabel != label) {
        return -1;
    }

    // decode kv pair and return it
    ret.first = ustring(&currKv[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&currKv[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
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
