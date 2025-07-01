#include <cstring>
#include <iomanip>
#include <sstream>

#include "enc_ind.h"


// this initializes everything to `\0`, i.e. zero bits
// technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv-pair, but currently `ENC_IND_KV_LEN` is 1024 bits so there's a 2^1024 chance of this happening
// USENIX'24's implementation also seems to just do this
static const uchar NULL_KV[ENC_IND_KV_LEN] = {};


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
/* `IEncIndLoc`                                                               */
/******************************************************************************/


template <class DbKw>
ulong IEncIndLoc<DbKw>::map(
    const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
) const {
    // `dbKwResCount` is equivalently the bucket size, which is 2^level
    long level = std::log2(dbKwResCount);
    long bucketStep = level >= 1 ? std::pow(2, level - 1) : 1;
    long bucketWithinLevel = (dbKwRange.first - minDbKw) / bucketStep;

    // a formula for the total number of items above level i, where n = leaf count and m = log_2 n is the level number
    // of the top level (where bottom level is 0) is (m - i)(2n) - (1 - 2^(i-m)) 2^(m+1); derivation below
    // the bucket count at level j (for j >= 1) is 2^(1-j)n - 1, and the bucket size at level j is 2^j
    // thus the total number of items above level i is
    //   (2^(1-m)n - 1)2^m  +  (2^(1-(m-1))n - 1)2^(m-1)  +  ...  +  (2^(1-(i+1))n - 1)2^(i+1)
    //           ^                         ^                                     ^
    //    items in level m         items in level m-1                    items in level i+1
    //
    // = (2n - 2^m) + (2n - 2^(m-1)) + ... + (2n - 2^(i+1))                                 (note there are m - i terms)
    // = (m - i)(2n) - (2^m + 2^(m-1) + ... + 2^(i+1))
    // = (m - i)(2n) - ((1 - 0.5^(m-i)) 2^m / 0.5)                                             (sum of geometric series)
    // = (m - i)(2n) - (1 - 2^(i-m)) 2^(m+1)
    long topLevel = std::log2(bottomLevelSize);
    ulong pos = (topLevel - level) * (2 * bottomLevelSize)
              - (1 - std::pow(2, level-topLevel)) * std::pow(2, topLevel+1);
    // add extra items in the same level before our current keyword, which consists of earlier buckets on our level
    // plus any earlier items in the same bucket (which we will use `rank` to determine)
    pos += bucketWithinLevel * std::pow(2, level);
    pos += rank;
    return pos;
}


/******************************************************************************/
/* `EncIndRamBase`                                                            */
/******************************************************************************/


void EncIndRamBase::initBase(long size) {
    this->arr = new uchar[size * ENC_IND_KV_LEN];
    // fill array with zero bits
    for (long i = 0; i < size; i++) {
        std::memcpy(&this->arr[i * ENC_IND_KV_LEN], NULL_KV, ENC_IND_KV_LEN);
    }
}


void EncIndRamBase::clearBase() {
    if (this->arr != nullptr) {
        delete[] this->arr;
        this->arr = nullptr;
    }
}


void EncIndRamBase::writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val) {
    ustring kv = label + val.first + val.second;
    std::memcpy(&this->arr[pos * ENC_IND_KV_LEN], kv.c_str(), ENC_IND_KV_LEN);
}


void EncIndRamBase::readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    uchar kv[ENC_IND_KV_LEN];
    std::memcpy(kv, &this->arr[pos * ENC_IND_KV_LEN], ENC_IND_KV_LEN);
    ret.first = ustring(&kv[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&kv[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
}


/******************************************************************************/
/* `EncIndDiskBase`                                                           */
/******************************************************************************/


void EncIndDiskBase::initBase(long size) {
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
        std::cerr << "EncIndDiskBase::initBase(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // fill file with zero bits
    for (long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(NULL_KV, ENC_IND_KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "EncIndDiskBase::initBase(): error initializing file (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


void EncIndDiskBase::clearBase() {
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


void EncIndDiskBase::writeToPos(ulong pos, const ustring& label, const std::pair<ustring, ustring>& val) {
    ustring kv = label + val.first + val.second;
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsWritten = std::fwrite(kv.c_str(), ENC_IND_KV_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "EncIndDiskBase::writeToPos(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::fflush(this->file);
}


void EncIndDiskBase::readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    uchar kv[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsRead = std::fread(kv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncIndDiskBase::readValFromPos(): error reading file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ret.first = ustring(&kv[ENC_IND_KEY_LEN], ENC_IND_DOC_LEN);
    ret.second = ustring(&kv[ENC_IND_KEY_LEN + ENC_IND_DOC_LEN], IV_LEN);
}


/******************************************************************************/
/* `EncIndRam`                                                                */
/******************************************************************************/


EncIndRam::~EncIndRam() {
    this->clear();
}


void EncIndRam::init(long size) {
    this->clear();
    this->initBase(size);
    this->size = size;
}


void EncIndRam::write(const ustring& label, const std::pair<ustring, ustring>& val) {
    // try to place encrypted items in the location specified by its encrypted label, e.g. PRF/hash output for PiBas 
    // (modulo buffer size); this seems iffy because of modulo but this is what USENIX'24's implementation does
    // (although they also use caching, presumably since it's slow if we need to keep finding next available locations)
    // this conversion mess is from USENIX'24's implementation
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currKv[ENC_IND_KV_LEN]; // I don't think we need a null terminator
    std::memcpy(currKv, &this->arr[pos * ENC_IND_KV_LEN], ENC_IND_KV_LEN);

    // if location is already filled (because of modulo), find next available location
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, NULL_KV, ENC_IND_KV_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        std::memcpy(currKv, &this->arr[pos * ENC_IND_KV_LEN], ENC_IND_KV_LEN);
    }
    if (std::memcmp(currKv, NULL_KV, ENC_IND_KV_LEN) != 0) {
        std::cerr << "EncIndRam::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // encode kv pair and write
    this->writeToPos(pos, label, val);
}


int EncIndRam::find(const ustring& label, std::pair<ustring, ustring>& ret) const {
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currLabel[ENC_IND_KEY_LEN];
    std::memcpy(currLabel, &this->arr[pos * ENC_IND_KV_LEN], ENC_IND_KEY_LEN);
    const uchar* labelCStr = label.c_str();

    // if location based on `label` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    long numPositionsChecked = 1;
    while (std::memcmp(currLabel, labelCStr, ENC_IND_KEY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        std::memcpy(currLabel, &this->arr[pos * ENC_IND_KV_LEN], ENC_IND_KEY_LEN);
    }
    // this does make it a lot slower to verify if an element is nonexistent compared to primary memory storage,
    // since we have to iterate through whole index
    // but this only makes reducing false positives all the more important
    if (std::memcmp(currLabel, labelCStr, ENC_IND_KEY_LEN) != 0) {
        // not found
        return -1;
    }

    // decode kv pair and return it
    this->readValFromPos(pos, ret);
    return 0;
}


void EncIndRam::clear() {
    this->clearBase();
}


/******************************************************************************/
/* `EncIndDisk`                                                               */
/******************************************************************************/


EncIndDisk::~EncIndDisk() {
    this->clear();
}


void EncIndDisk::init(long size) {
    this->clear();
    this->initBase(size);
    this->size = size;
}


void EncIndDisk::write(const ustring& label, const std::pair<ustring, ustring>& val) {
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currKv[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "EncIndDisk::write(): error reading file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, NULL_KV, ENC_IND_KV_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "EncIndDisk::write(): error reading file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, NULL_KV, ENC_IND_KV_LEN) != 0) {
        std::cerr << "EncIndDisk::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // encode kv pair and write
    this->writeToPos(pos, label, val);
}


int EncIndDisk::find(const ustring& label, std::pair<ustring, ustring>& ret) const {
    ulong pos = (*((ulong*)label.c_str())) % this->size;
    uchar currKv[ENC_IND_KV_LEN];
    std::fseek(this->file, pos * ENC_IND_KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncIndDisk::find(): error reading file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    const uchar* labelCStr = label.c_str();

    // if location based on `label` did not match the target (i.e. another kv pair overflowed to here first), scan
    // subsequent locations for where the target could've overflowed to
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, labelCStr, ENC_IND_KEY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, ENC_IND_KV_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "EncIndDisk::find(): error reading file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, labelCStr, ENC_IND_KEY_LEN) != 0) {
        // not found
        return -1;
    }

    // decode kv pair and return it
    this->readValFromPos(pos, ret);
    return 0;
}


void EncIndDisk::clear() {
    this->clearBase();
}


/******************************************************************************/
/* `EncIndLocRam`                                                             */
/******************************************************************************/


template <class DbKw>
EncIndLocRam<DbKw>::~EncIndLocRam() {
    this->clear();
}


template <class DbKw>
void EncIndLocRam<DbKw>::init(long size) {
    this->clear();
    this->initBase(size);
}


template <class DbKw>
void EncIndLocRam<DbKw>::write(
    const ustring& label, const std::pair<ustring, ustring>& val,
    const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
) {
    ulong pos = this->map(dbKwRange, dbKwResCount, rank, minDbKw, bottomLevelSize);
    this->writeToPos(pos, label, val);
}


template <class DbKw>
void EncIndLocRam<DbKw>::find(
    const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
    std::pair<ustring, ustring>& ret
) const {
    ulong pos = this->map(dbKwRange, dbKwResCount, rank, minDbKw, bottomLevelSize);
    this->readValFromPos(pos, ret);
}


template <class DbKw>
void EncIndLocRam<DbKw>::clear() {
    this->clearBase();
}


template class EncIndLocRam<Kw>;
//template class EncIndLocRam<IdAlias>;


/******************************************************************************/
/* `EncIndLocDisk`                                                            */
/******************************************************************************/


template <class DbKw>
EncIndLocDisk<DbKw>::~EncIndLocDisk() {
    this->clear();
}


template <class DbKw>
void EncIndLocDisk<DbKw>::init(long size) {
    this->clear();
    this->initBase(size);
}


template <class DbKw>
void EncIndLocDisk<DbKw>::write(
    const ustring& label, const std::pair<ustring, ustring>& val,
    const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize
) {
    ulong pos = this->map(dbKwRange, dbKwResCount, rank, minDbKw, bottomLevelSize);
    this->writeToPos(pos, label, val);
}


template <class DbKw>
void EncIndLocDisk<DbKw>::find(
    const Range<DbKw>& dbKwRange, long dbKwResCount, long rank, DbKw minDbKw, long bottomLevelSize,
    std::pair<ustring, ustring>& ret
) const {
    ulong pos = this->map(dbKwRange, dbKwResCount, rank, minDbKw, bottomLevelSize);
    this->readValFromPos(pos, ret);
}


template <class DbKw>
void EncIndLocDisk<DbKw>::clear() {
    this->clearBase();
}

template class EncIndLocDisk<Kw>;
//template class EncIndLocDisk<IdAlias>;
