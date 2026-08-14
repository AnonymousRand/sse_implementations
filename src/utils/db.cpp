#include "utils/db.cpp"

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "utils/disk_storage.h"
#include "utils/tuple.h"


template <IsDbTuple DbTuple>
Db<DbTuple>::Db() {
    this->initFile();
}


template <IsDbTuple DbTuple>
Db<DbTuple>::Db(const Db& db) {
    this->filename = this->genFilename();
    this->_size = db->_size;

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

    std::fseek(this->file, 0, SEEK_END);
    int itemsWritten = std::fwrite(dbTupleStr.c_str(), TUPLE_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: Db::push_back(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->_size++;
}


template <IsDbTuple DbTuple>
const DbTuple& Db<DbTuple>::operator [](int64_t index) const {
    if (index >= this->_size) {
        std::cerr << "Error: Db::operator []: index out of bounds "
                  << "(index is " << index << ", size is " << this->_size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    char dbTupleCstr[TUPLE_LEN];
    std::fseek(this->file, index * TUPLE_LEN, SEEK_SET);
    int itemsWritten = std::fread(dbTupleCstr, TUPLE_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "Error: Db::operator []: error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // unpad as necessary so that decoding works properly
    int paddingStart;
    for (paddingStart = TUPLE_LEN - 1; paddingStart >= 0; paddingStart--) {
        if (dbTupleStr[paddingStart] != '\0') {
            break;
        }
    }
    std::string dbTupleStr(dbTupleCstr, paddingStart);

    // decode and return
    return DbTuple::fromStr(dbTupleStr);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Db<Tuple<>>;        // default/input DBs
template class Db<SrcIDb1Tuple>;   // Log-SRC-i index 1 DBs
//template class Db<Tuple<IdAlias>>; // Log-SRC-i index 2 DBs
