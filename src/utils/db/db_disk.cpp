#include "utils/db/db.h"

#include <algorithm>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "utils/disk_storage.h"
#include "utils/misc.h"
#include "utils/random.h"
#include "utils/tuple.h"
#include "utils/types.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk() {
    IDiskStorage::init();
}


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(const DbDisk<DbTuple>& db, bigint startIndex, bigint endIndex) :
    DbDisk<DbTuple>()
{
    for (bigint index = startIndex; index < endIndex; index++) {
        // avoid decoding and re-encoding every tuple, which incurs expensive regex
        char dbTupleCstr[TUPLE_LEN];
        db.readRaw(index, dbTupleCstr);
        this->appendRaw(dbTupleCstr);
    }
}


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(std::initializer_list<DbTuple> initList) :
    DbDisk<DbTuple>()
{
    for (const DbTuple& dbTuple : initList) {
        this->push_back(dbTuple);
    }
}


//------------------------------------------------------------------------------
// copy/move


// copy constructor
template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(const DbDisk& other) :
    IDb<DbTuple>(other)
{
    IDiskStorage::copyFrom(other);
}


//------------------------------------------------------------------------------
// `IDb`


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::clear() {
    // clears `this->_size`
    IDb<DbTuple>::clear();

    // clears DB file and file pointer
    IDiskStorage::clear();
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::push_back(const DbTuple& dbTuple) {
    std::string dbTupleStr = dbTuple.toStr();

    // make sure every encoded tuple is stored into the same fixed-length size for easy lookups
    // pad with null bytes if necessary, like a null terminator for a string in memory
    if (dbTupleStr.length() > TUPLE_LEN) {
        std::cerr << "Error: DbDisk::push_back(): write of length " << dbTupleStr.length()
                  << " bytes is not allowed! "
                  << "(want " << TUPLE_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    utils::padStr(dbTupleStr, TUPLE_LEN);

    this->appendRaw(dbTupleStr.c_str());
}


template <IsDbTuple DbTuple>
DbTuple DbDisk<DbTuple>::operator [](bigint index) const {
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
    dbTupleStr.resize(paddingStart + 1); // (`+ 1` to add back first null terminator)

    // decode and return
    return DbTuple::fromStr(dbTupleStr);
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::shuffle() {
    auto shuffle = [](std::vector<bigint>& dbIndices) {
        std::shuffle(dbIndices.begin(), dbIndices.end(), utils::RNG);
    };
    *this = this->applyAlgoViaIndices(shuffle);
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::sort(
    const std::function<bool(const DbTuple& dbTuple1, const DbTuple& dbTuple2)>& compare
) {
    // since we actually need to sort on the indices in this case
    auto compareIndices = [this, &compare](bigint index1, bigint index2) {
        return compare((*this)[index1], (*this)[index2]);
    };

    auto sort = [&compareIndices](std::vector<bigint>& dbIndices) {
        std::sort(dbIndices.begin(), dbIndices.end(), compareIndices);
    };
    *this = this->applyAlgoViaIndices(sort);
}


//--------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::readRaw(bigint index, char* ret) const {
    if (index >= this->_size) {
        std::cerr << "Error: DbDisk::readRaw(): index out of bounds "
                  << "(index is " << index << ", size is " << this->_size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // make sure to flush if more writes have been done since the last manual flush
    if (!this->isFlushed) {
        std::fflush(this->file);
        this->isFlushed = true;
    }

    std::fseek(this->file, index * TUPLE_LEN, SEEK_SET);
    int itemsRead = std::fread(ret, TUPLE_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "Error: DbDisk::readRaw(): error reading from file " << this->filename
                  << " (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::appendRaw(const char* dbTupleCstr) {
    std::fseek(this->file, 0, SEEK_END);
    int itemsWritten = std::fwrite(dbTupleCstr, TUPLE_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: DbDisk::appendRaw(): error writing to file " << this->filename
                  << " (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->isFlushed = false;

    this->_size++;
}


template <IsDbTuple DbTuple>
DbDisk<DbTuple> DbDisk<DbTuple>::applyAlgoViaIndices(
    const std::function<void(std::vector<bigint>& dbIndices)>& algoOnIndices
) const {
    std::vector<bigint> dbIndices;
    dbIndices.reserve(this->_size);
    for (bigint index = 0; index < this->_size; index++) {
        dbIndices.push_back(index);
    }

    algoOnIndices(dbIndices);

    // now build output DB using this vector of indices
    DbDisk<DbTuple> outputDbDisk;
    for (bigint index : dbIndices) {
        outputDbDisk.push_back((*this)[index]);
    }
    return outputDbDisk;
}


//------------------------------------------------------------------------------
// explicit template instantiations


// >TODO: can move these into db.cpp? using just Db<...>
template class DbDisk<Tuple<>>;
template class DbDisk<SrcIDb1Tuple>;
//template class DbDisk<Tuple<IdAlias>>;
