#include "utils/db.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

#include "utils/disk_storage.h"
#include "utils/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
Db<DbTuple>::Db() {
    this->initFile();
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(const Db& db) {
    this->filename = this->genFilename();
    this->_size = db._size;

    try {
        std::filesystem::copy_file(db.filename, this->filename);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: Db::Db(const Db& db): error copying file: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->file = std::fopen(this->filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error: Db::Db(const Db& db): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(const Db& db, int64_t startIndex, int64_t endIndex) {
    this->filename = this->genFilename();
    this->_size = endIndex - startIndex;

    for (int64_t index = startIndex; index < endIndex; index++) {
        std::string dbTupleStr = db.readRaw(index);
        this->writeRaw(dbTupleStr);
    }
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(std::initializer_list<DbTuple> initList) : Db<DbTuple>() {
    for (DbTuple dbTuple : initList) {
        this->push_back(dbTuple);
    }
}


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
void Db<DbTuple>::clear() {
    IDiskStorage::clear();

    this->_size = 0;
}


template <IsDbTuple DbTuple>
void Db<DbTuple>::push_back(const DbTuple& dbTuple) {
    std::string dbTupleStr = dbTuple.toStr();

    // make sure every encoded tuple is stored into the same fixed-length size for easy lookups
    // pad with null bytes if necessary, like a null terminator for a string in memory
    if (dbTupleStr.length() > TUPLE_LEN) {
        std::cerr << "Error: Db::push_back(): write of length " << dbTupleStr.length()
                  << " bytes is not allowed! "
                  << "(want " << TUPLE_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (dbTupleStr.length() < TUPLE_LEN) {
        int amountToPad = TUPLE_LEN - dbTupleStr.length();
        std::string padding(amountToPad, '\0');
        dbTupleStr += padding;
    }

    this->writeRaw(dbTupleStr);
}


template <IsDbTuple DbTuple>
DbTuple Db<DbTuple>::operator [](int64_t index) const {
    std::string dbTupleStr = this->readRaw(index);

    // unpad as necessary so that decoding works properly
    int paddingStart;
    for (paddingStart = TUPLE_LEN - 1; paddingStart >= 0; paddingStart--) {
        if (dbTupleStr[paddingStart] != '\0') {
            break;
        }
    }
    dbTupleStr.resize(paddingStart + 1); // (`+ 1` for null terminator)

    // decode and return
    return DbTuple::fromStr(dbTupleStr);
}


//--------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
std::string Db<DbTuple>::readRaw(int64_t index) const {
    if (index >= this->_size) {
        std::cerr << "Error: Db::readRaw(): index out of bounds "
                  << "(index is " << index << ", size is " << this->_size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    char dbTupleCstr[TUPLE_LEN];
    std::fseek(this->file, index * TUPLE_LEN, SEEK_SET);
    int itemsRead = std::fread(dbTupleCstr, TUPLE_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: Db::readRaw(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return std::string(dbTupleCstr);
}


template <IsDbTuple DbTuple>
void Db<DbTuple>::writeRaw(const std::string& dbTupleStr) {
    std::fseek(this->file, 0, SEEK_END);
    int itemsWritten = std::fwrite(dbTupleStr.c_str(), TUPLE_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: Db::writeRaw(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->_size++;
}


template <IsDbTuple DbTuple>
Db<DbTuple> Db<DbTuple>::applyAlgoViaIndices(
    const std::function<void(const std::vector<int64_t>& dbIndices)>& algoOnIndices
) const {
    std::vector<int64_t> dbIndices;
    dbIndices.reserve(this->_size);
    for (int64_t index = 0; index < this->_size; index++) {
        dbIndices.push_back(index);
    }

    algo(dbIndices);

    // now build output DB using this vector of indices
    Db<DbTuple> outputDb;
    for (int64_t index : dbIndices) {
        outputDb.push_back((*this)[index]);
    }
    return outputDb;
}



template <IsDbTuple DbTuple>
void Db<DbTuple>::shuffle() {
    auto shuffle = [&RNG](const std::vector<int64_t>& dbIndices) {
        std::shuffle(dbIndices.begin(), dbIndices.end(), RNG);
    };
    *this = this->applyAlgoViaIndices(shuffle);
}


template <IsDbTuple DbTuple>
void Db<DbTuple>::sort(const std::function<void(int64_t index1, int64_t index2)>& compare) {
    auto sort = [&compare](const std::vector<int64_t>& dbIndices) {
        std::sort(dbIndices.begin(), dbIndices.end(), compare);
    };
    *this = this->applyAlgoViaIndices(sort);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Db<Tuple<>>;        // default/input DBs
template class Db<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class Db<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs
