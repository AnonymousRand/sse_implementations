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
#include <random>
#include <string>
#include <vector>

#include "utils/disk_storage.h"
#include "utils/random.h"
#include "utils/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
Db<DbTuple>::Db() {
    //std::cerr << "db default constructor" << std::endl;
    IDiskStorage::init();
}


// TODO minor: make sure other copy constructors have param name "other" too
template <IsDbTuple DbTuple>
Db<DbTuple>::Db(const Db& other) {
    //std::cerr << "db copy constructor" << std::endl;
    this->filename = this->genFilename();
    this->_size = other._size;
    //std::cerr << "other.filename is " << other.filename << "; this->filename is " << this->filename << std::endl;

    // flush to make sure `other.file` has written everything
    std::fflush(other.file);
    try {
        std::filesystem::copy_file(other.filename, this->filename);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: Db::Db(const Db&): error copying file: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    //std::cerr << "copied to " << this->filename << "; our size is " << this->_size << std::endl;

    // note: use `a` instead of `w` mode always here to not overwrite the file we just copied
    this->file = std::fopen(this->filename.c_str(), "ab+");
    if (this->file == nullptr) {
        std::cerr << "Error: Db::Db(const Db&): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(const Db& db, int64_t startIndex, int64_t endIndex) : Db<DbTuple>() {
    //std::cerr << "db range constructor from " << db.filename << " to " << this->filename << std::endl;
    //std::cerr << "size is " << this->_size << "; original db size was " << db.size() << "; file is " << this->file << ", " << this->filename << std::endl;

    for (int64_t index = startIndex; index < endIndex; index++) {
        //std::cerr << "writing " << index << std::endl;
        char dbTupleCstr[TUPLE_LEN];
        db.readRaw(index, dbTupleCstr);
        //std::cerr << "writing2 " << dbTupleCstr << std::endl;
        this->appendRaw(dbTupleCstr);
    }
    //std::cerr << "done writing " << std::endl;
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(std::initializer_list<DbTuple> initList) : Db<DbTuple>() {
    //std::cerr << "db init list constructor" << std::endl;
    for (DbTuple dbTuple : initList) {
        //std::cerr << "db init list constructor pushing back" << std::endl;
        this->push_back(dbTuple);
    }
    //std::cerr << "done init list constructor" << std::endl;
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

    this->appendRaw(dbTupleStr.c_str());
}


template <IsDbTuple DbTuple>
bool Db<DbTuple>::empty() const {
    return this->_size == 0;
}


template <IsDbTuple DbTuple>
DbTuple Db<DbTuple>::operator [](int64_t index) const {
    char dbTupleCstr[TUPLE_LEN];
    this->readRaw(index, dbTupleCstr);
    std::string dbTupleStr(dbTupleCstr, TUPLE_LEN);

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
void Db<DbTuple>::readRaw(int64_t index, char* ret) const {
    //std::cerr << "readRaw " << index << ", this->size is " << this->_size << "; file is " << this->file << ", " << this->filename << std::endl;
    if (index >= this->_size) {
        std::cerr << "Error: Db::readRaw(): index out of bounds "
                  << "(index is " << index << ", size is " << this->_size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::fseek(this->file, index * TUPLE_LEN, SEEK_SET);
    int itemsRead = std::fread(ret, TUPLE_LEN, 1, this->file);
    //std::cerr << "done read" << std::endl;
    if (itemsRead != 1) {
        std::cerr << "Error: Db::readRaw(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


template <IsDbTuple DbTuple>
void Db<DbTuple>::appendRaw(const char* dbTupleCstr) {
    //std::cerr << "appendRaw " << dbTupleStr << "; this->file is " << this->file << std::endl;
    std::fseek(this->file, 0, SEEK_END);
    //std::cerr << "appendRaw 2" << dbTupleStr << std::endl;
    int itemsWritten = std::fwrite(dbTupleCstr, TUPLE_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: Db::appendRaw(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    //std::cerr << "db appendRaw wrote to " << this->filename << ", pointer " << this->file << std::endl;
    std::fflush(this->file);

    this->_size++;
}


template <IsDbTuple DbTuple>
Db<DbTuple> Db<DbTuple>::applyAlgoViaIndices(
    const std::function<void(std::vector<int64_t>& dbIndices)>& algoOnIndices
) const {
    std::vector<int64_t> dbIndices;
    dbIndices.reserve(this->_size);
    for (int64_t index = 0; index < this->_size; index++) {
        dbIndices.push_back(index);
    }

    algoOnIndices(dbIndices);

    // now build output DB using this vector of indices
    Db<DbTuple> outputDb;
    for (int64_t index : dbIndices) {
        outputDb.push_back((*this)[index]);
    }
    return outputDb;
}



template <IsDbTuple DbTuple>
void Db<DbTuple>::shuffle() {
    auto shuffle = [](std::vector<int64_t>& dbIndices) {
        std::shuffle(dbIndices.begin(), dbIndices.end(), utils::RNG);
    };
    *this = this->applyAlgoViaIndices(shuffle);
}


template <IsDbTuple DbTuple>
void Db<DbTuple>::sort(const std::function<bool(int64_t index1, int64_t index2)>& compare) {
    auto sort = [&compare](std::vector<int64_t>& dbIndices) {
        std::sort(dbIndices.begin(), dbIndices.end(), compare);
    };
    *this = this->applyAlgoViaIndices(sort);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Db<Tuple<>>;        // default/input DBs
template class Db<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class Db<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs
