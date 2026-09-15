#include "utils/types/db/db.h"

#include <algorithm>
#include <cassert>
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

#include "config.h"

#include "utils/debug.h"
#include "utils/misc.h"
#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/i_db.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk() {
    // inits DB file and file pointer
    IDiskStorage<char>::init();
}


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(const DbDisk<DbTuple>& other, bigint startIndex, bigint endIndex) :
    // important to always call default constructor so `IDiskStorage::init()` can be called!
    DbDisk<DbTuple>()
{
    for (bigint index = startIndex; index < endIndex; index++) {
        DbTuple dbTuple = other[index];
        this->append(dbTuple);
    }
}


template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(std::initializer_list<DbTuple> initList) :
    DbDisk<DbTuple>()
{
    for (const DbTuple& dbTuple : initList) {
        this->append(dbTuple);
    }
}


//------------------------------------------------------------------------------
// rule of five


// copy constructor
template <IsDbTuple DbTuple>
DbDisk<DbTuple>::DbDisk(const DbDisk& other) :
    // call `IDb`'s base copy constructor to ensure it gets run as well
    IDb<DbTuple>(other)
{
    IDiskStorage<char>::copyFrom(other);
}


//------------------------------------------------------------------------------
// `IDb`


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::clear() {
    // clears `this->size`
    IDb<DbTuple>::clear();

    // clears DB file and file pointer
    IDiskStorage<char>::clear();
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::append(const DbTuple& dbTuple) {
    assert(this->file != nullptr);
    std::string dbTupleStr = dbTuple.toStr();

    // make sure every encoded tuple is stored into the same fixed-length size for easy lookups,
    // padding with '\0' bytes if necessary
    DEBUG_ONLY({
        if (dbTupleStr.length() > config::TUPLE_ENCOD_LEN) {
            std::cerr << "Error: DbDisk::append(): write of length " << dbTupleStr.length()
                      << " bytes is not allowed! (want " << config::TUPLE_ENCOD_LEN << " bytes)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
    utils::misc::padStr(dbTupleStr, config::TUPLE_ENCOD_LEN);

    // write to DB
    std::fseek(this->file, 0, SEEK_END);
    this->writeToFile(dbTupleStr.c_str(), config::TUPLE_ENCOD_LEN, 1, "DbDisk::append()");

    // update member variables as needed
    this->onNewDbTuple(dbTuple);
}


template <IsDbTuple DbTuple>
DbTuple DbDisk<DbTuple>::operator [](bigint index) const {
    assert(this->file != nullptr);
    assert(index < this->size);

    // read from DB
    char dbTupleCstr[config::TUPLE_ENCOD_LEN];
    std::fseek(this->file, index * config::TUPLE_ENCOD_LEN, SEEK_SET);
    this->readFromFile(dbTupleCstr, config::TUPLE_ENCOD_LEN, 1, "DbDisk::operator []");
    std::string dbTupleStr(dbTupleCstr, config::TUPLE_ENCOD_LEN);

    // unpad as necessary so that decoding works properly
    utils::misc::unpadStr(dbTupleStr);

    // decode and return
    return DbTuple::fromStr(dbTupleStr);
}


template <IsDbTuple DbTuple>
void DbDisk<DbTuple>::shuffle() {
    auto shuffle = [](std::vector<bigint>& dbIndices) {
        std::shuffle(dbIndices.begin(), dbIndices.end(), utils::random::RNG);
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
DbDisk<DbTuple> DbDisk<DbTuple>::applyAlgoViaIndices(
    const std::function<void(std::vector<bigint>& dbIndices)>& algoOnIndices
) const {
    assert(this->file != nullptr);
    std::vector<bigint> dbIndices;
    dbIndices.reserve(this->size);
    for (bigint index = 0; index < this->size; index++) {
        dbIndices.push_back(index);
    }

    algoOnIndices(dbIndices);

    // now build output DB using this vector of indices
    DbDisk<DbTuple> outputDbDisk;
    for (bigint index : dbIndices) {
        outputDbDisk.append((*this)[index]);
    }
    return outputDbDisk;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class DbDisk<Tuple<>>;
template class DbDisk<SrcIDb1Tuple>;
//template class DbDisk<Tuple<IdAlias>>;
