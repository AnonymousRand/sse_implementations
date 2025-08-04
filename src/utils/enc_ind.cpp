#include "enc_ind.h"


/******************************************************************************/
/* `EncIndBase`                                                               */
/******************************************************************************/


// this initializes everything to `\0`, i.e. zero bits
// technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv-pair, but currently `EncIndBase::KV_LEN` is 1024 bits so there's a 2^1024 chance of this happening
// USENIX'24's implementation also seems to just do this
const uchar EncIndBase::NULL_KV[EncIndBase::KV_LEN] = {};


EncIndBase::~EncIndBase() {
    this->clear();
}


void EncIndBase::init(long size) {
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
        std::cerr << "EncIndBase::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // fill file with zero bits
    for (long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(EncIndBase::NULL_KV, EncIndBase::KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "EncIndBase::init(): error initializing file (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


void EncIndBase::clear() {
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr; // important for idempotence!
    }
    // delete encrypted index files from disk on `clear()`
    if (this->filename != "") {
        std::remove(this->filename.c_str());
        this->filename = "";
    }
    this->size = 0;
}


bool EncIndBase::readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    uchar kv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(kv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncIndBase::readValFromPos(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(kv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) == 0) {
        // if `pos` contains `NULL_KV`
        return false;
    }

    ret.first = ustring(&kv[EncIndBase::KEY_LEN], EncIndBase::DOC_LEN);
    ret.second = ustring(&kv[EncIndBase::KEY_LEN + EncIndBase::DOC_LEN], IV_LEN);
    return true;
}


void EncIndBase::writeToPos(
    ulong pos, const ustring& key, const std::pair<ustring, ustring>& val, bool flushImmediately
) {
    if (pos >= this->size) {
        std::cerr << "EncIndBase::writeToPos(): write to pos " << pos << " is out of bounds! "
                  << "(size is " << this->size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ustring kv = key + val.first + val.second;
    if (kv.length() != EncIndBase::KV_LEN) {
        std::cerr << "EncIndBase::writeToPos(): write of length " << kv.length() << " bytes is too long! "
                  << "(want " << EncIndBase::KV_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsWritten = std::fwrite(kv.c_str(), EncIndBase::KV_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "EncIndBase::writeToPos(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (flushImmediately) {
        std::fflush(this->file);
    }
}


/******************************************************************************/
/* `EncInd`                                                                   */
/******************************************************************************/


void EncInd::write(const ustring& key, const std::pair<ustring, ustring>& val) {
    // this conversion mess is from USENIX'24
    ulong pos = (*((ulong*)key.c_str())) % this->size;
    uchar currKv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "EncInd::write(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if location is already filled (because of modulo), find next available location
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

    // encode kv pair and write (flush immediately to mark space as occupied)
    this->writeToPos(pos, key, val, true);
}


bool EncInd::find(const ustring& key, std::pair<ustring, ustring>& ret) const {
    ulong pos = (*((ulong*)key.c_str())) % this->size;
    uchar currKv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncInd::find(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    const uchar* keyCStr = key.c_str();

    // if location based on `key` did not match the target `key` (i.e. another kv pair overflowed to here first),
    // scan subsequent locations for where the target `key` could've overflowed to
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, keyCStr, EncIndBase::KEY_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsRead = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
        if (itemsRead != 1) {
            std::cerr << "EncInd::find(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, keyCStr, EncIndBase::KEY_LEN) != 0) {
        // if not found
        return false;
    }

    // decode kv pair and return it
    this->readValFromPos(pos, ret);
    return true;
}


/******************************************************************************/
/* `EncIndLoc`                                                                */
/******************************************************************************/


template <class DbKw>
void EncIndLoc<DbKw>::write(
    const ustring& key, const std::pair<ustring, ustring>& val,
    const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
) {
    ulong pos = this->map(dbKwRange, dbKwCount, dbKwCounter, minDbKw, bottomLevelSize);
    this->writeToPos(pos, key, val);
}


template <class DbKw>
void EncIndLoc<DbKw>::find(
    const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize,
    std::pair<ustring, ustring>& ret
) const {
    ulong pos = this->map(dbKwRange, dbKwCount, dbKwCounter, minDbKw, bottomLevelSize);
    this->readValFromPos(pos, ret);
}


template <class DbKw>
ulong EncIndLoc<DbKw>::map(
    const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
) {
    // `dbKwCount` is equivalently the bucket size, which is `2^levelNum`
    long levelNum = std::log2(dbKwCount);
    long bucketStep = levelNum >= 1 ? std::pow(2, levelNum - 1) : 1;
    long bucketWithinLevel = (dbKwRange.first - minDbKw) / bucketStep;

    // a formula for the total number of items above level i, where n = leaf count and m = log_2(n) is the level number
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
    long topLevelNum = std::log2(bottomLevelSize);
    ulong pos = (topLevelNum - levelNum) * (2 * bottomLevelSize)
              - (1 - std::pow(2, levelNum - topLevelNum)) * std::pow(2, topLevelNum + 1);
    // add extra items in the same level before our current keyword, which consists of earlier buckets on our level
    // plus any earlier items in the same bucket (which we will use `dbKwCounter` to determine)
    pos += bucketWithinLevel * std::pow(2, levelNum);
    pos += dbKwCounter;
    return pos;
}


template class EncIndLoc<Kw>;
//template class EncIndLoc<IdAlias>;
