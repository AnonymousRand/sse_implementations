#include "enc_ind_pibas.h"

#include "utils/cryptography.h"


template <class DbDoc, class Dbkw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndPibas<DbDoc, DbKw>::init(long size) {
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
        std::cerr << "EncIndPibas::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // fill file with zero bits
    for (long i = 0; i < size; i++) {
        int itemsWritten = std::fwrite(EncIndBase::NULL_KV, EncIndBase::KV_LEN, 1, this->file);
        if (itemsWritten != 1) {
            std::cerr << "EncIndPibas::init(): error initializing file (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}


template <class DbDoc, class Dbkw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndPibas<DbDoc, DbKw>::clear() {
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


template <class DbDoc, class Dbkw> requires IsValidDbParams<DbDoc, DbKw>
bool EncIndPibas<DbDoc, DbKw>::find(
    const ustring& prfKey, const ustring& encKey, const Range<DbKw>& query, long dbKwCounter,
    std::pair<ustring, ustring>& ret
) const {
    EncIndEntry encIndEntry;
    ulong pos = this->map(prfKey, encKey, );
    uchar currKv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncIndPibas::find(): error reading from file (nothing read)" << std::endl;
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
            std::cerr << "EncIndPibas::find(): error reading from file (nothing read)" << std::endl;
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


template <class DbDoc, class Dbkw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndPibas<DbDoc, DbKw>::write(
    const ustring& prfKey, const ustring& encKey, DbDoc dbDoc, const Range<DbKw>& dbKwRange, long dbKwCounter,
    const std::pair<ustring, ustring>& val
) {
    // generate encrypted entry and position to write it
    EncIndEntry endIndEntry;
    ulong pos = this->map(prfKey, encKey, dbDoc, dbKwRange, dbKwCounter, encIndEntry);
    // d <- Enc(K_2, w, id)
    ustring iv = genIv(IV_LEN);
    ustring encDbDoc = padAndEncrypt(ENC_CIPHER, encKey, dbDoc.toUstr(), iv, EncIndBase::DOC_LEN - 1);
    uchar currKv[EncIndBase::KV_LEN];

    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsReadOrWritten = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsReadOrWritten != 1) {
        std::cerr << "EncIndPibas::write(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // if position is already filled (because of modulo), find next available position
    long numPositionsChecked = 1;
    while (std::memcmp(currKv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) != 0 && numPositionsChecked < this->size) {
        numPositionsChecked++;
        pos = (pos + 1) % this->size;
        if (pos == 0) {
            std::fseek(this->file, 0, SEEK_SET);
        }
        itemsReadOrWritten = std::fread(currKv, EncIndBase::KV_LEN, 1, this->file); 
        if (itemsReadOrWritten != 1) {
            std::cerr << "EncIndPibas::write(): error reading from file (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    if (std::memcmp(currKv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) != 0) {
        std::cerr << "EncIndPibas::write(): ran out of space writing!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // write (and encode) encrypted entry (flush immediately to mark space as occupied)
    this->writeToPos(pos, key, val, true);
}


template <class DbDoc, class Dbkw> requires IsValidDbParams<DbDoc, DbKw>
ulong EncIndPibas<DbDoc, DbKw>::map(
    const ustring& prfKey, const ustring& encKey, DbDoc dbDoc, const Range<DbKw>& dbKwRange, long dbKwCounter,
    ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w) || c)
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
    // this conversion mess is from USENIX'24
    return (*((ulong*)retLabel.c_str())) % this->size;
}
