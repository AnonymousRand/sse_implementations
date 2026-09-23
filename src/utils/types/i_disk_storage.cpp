#include "utils/types/i_disk_storage.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <random>
#include <string>
#include <utility>

#include "utils/debug.h"
#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// constructors/destructors


template <class CharType>
IDiskStorage<CharType>::~IDiskStorage() {
    this->clear();
}


//------------------------------------------------------------------------------
// rule of five


template <class CharType>
void IDiskStorage<CharType>::copyFrom(const IDiskStorage& other) {
    this->filename = this->genFilename();
    // flush if needed to make sure `other.file` has written everything
    if (!other.isFlushed) {
        std::fflush(other.file);
        other.isFlushed = true;
    }

    // directly copy the DB file from `other` to `this`'s new filename via system call
    try {
        std::filesystem::copy_file(other.filename, this->filename);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: IDiskStorage::copyFrom(): error copying file " << this->filename
                  << ": " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // open the file we just copied
    // we use `a` instead of `w` mode here to not overwrite the file we just copied
    this->file = std::fopen(this->filename.c_str(), "ab+");
    DEBUG_ONLY({
        if (this->file == nullptr) {
            std::perror(std::format(
                "Error: IDiskStorage::copyFrom(): error opening file {}",
                this->filename
            ).c_str());
            std::exit(EXIT_FAILURE);
        }
    });
}


template <class CharType>
void IDiskStorage<CharType>::moveFrom(IDiskStorage&& other) noexcept {
    // IMPORTANT: set all fields in `other` that have non-default destruction/should not be
    // double-freed to a null value, so that its destructor doesn't try to delete the same resource
    // (e.g.  pointer or filename) that `this`'s fields now point to when it goes out of scope
    this->file = other.file;
    other.file = nullptr;

    this->filename = other.filename;
    other.filename = "";

    this->isFlushed = other.isFlushed;
}


// copy assignment operator
template <class CharType>
IDiskStorage<CharType>& IDiskStorage<CharType>::operator =(const IDiskStorage& other) {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->copyFrom(other);
    }
    return *this;
}


// move constructor
template <class CharType>
IDiskStorage<CharType>::IDiskStorage(IDiskStorage&& other) noexcept {
    this->moveFrom(std::move(other));
}


// move assignment operator
template <class CharType>
IDiskStorage<CharType>& IDiskStorage<CharType>::operator =(IDiskStorage&& other) noexcept {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->moveFrom(std::move(other));
    }
    return *this;
}


//------------------------------------------------------------------------------
// interface


template <class CharType>
void IDiskStorage<CharType>::init() {
    this->clear();

    // first make sure base directory exists
    try {
        std::filesystem::create_directories(this->FILE_DIR());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: IDiskStorage::init(): error creating path " << this->FILE_DIR() << ": "
                  << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->filename = this->genFilename();

    // while file exists (or any other error occurs on open), create new random filename
    while (std::filesystem::exists(this->filename)) {
        this->filename = this->genFilename();
    }
    
    this->file = std::fopen(this->filename.c_str(), "wb+");
    DEBUG_ONLY({
        if (this->file == nullptr) {
            std::perror(std::format(
                "Error: IDiskStorage::init(): error opening file {}",
                this->filename
            ).c_str());
            std::exit(EXIT_FAILURE);
        }
    });
}


template <class CharType>
void IDiskStorage<CharType>::clear() {
    // close file descriptors
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }

    // delete file from disk
    if (this->filename != "") {
        try {
            std::filesystem::remove(this->filename);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: IDiskStorage::clear(): error removing file " << this->filename
                      << ": " << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        this->filename = "";
    }
}


//--------------------------------------------------------------------------
// helpers


template <class CharType>
bigint IDiskStorage<CharType>::readFromFile(
    CharType* ret, bigint length, bigint count, const std::string& caller
) const {
    // make sure to flush if more writes have been done since the last manual flush
    this->flushIfNotFlushed();

    bigint itemsRead = std::fread(ret, length, count, this->file);
    DEBUG_ONLY({
        if (itemsRead != count) {
            std::perror(std::format(
                "Error: {}: error reading from file {} (only {} out of {} read)",
                caller, this->filename, itemsRead, count
            ).c_str());
            std::exit(EXIT_FAILURE);
        }
    });

    return itemsRead;
}


template <class CharType>
bigint IDiskStorage<CharType>::writeToFile(
    const CharType* toWrite, bigint length, bigint count, const std::string& caller
) {
    bigint itemsWritten = std::fwrite(toWrite, length, count, this->file);
    DEBUG_ONLY({
        if (itemsWritten != count) {
            std::perror(std::format(
                "Error: {}: error writing to file {} (only {} out of {} written)",
                caller, this->filename, itemsWritten, count
            ).c_str());
            std::exit(EXIT_FAILURE);
        }
    });

    this->isFlushed = false;
    return itemsWritten;
}


template <class CharType>
std::string IDiskStorage<CharType>::genFilename() const {
    // avoid naming clashes by generating a random 8 byte (16 char) hex string
    std::uniform_int_distribution<ubigint> dist;
    ubigint randomHex = dist(utils::random::RNG);
    std::string randomHexStr = std::format("{:016x}", randomHex);
    return std::format("{}/{}{}.dat", this->FILE_DIR(), this->FILENAME_PREFIX(), randomHexStr);
}


template <class CharType>
void IDiskStorage<CharType>::flushIfNotFlushed() const {
    assert(this->file != nullptr);
    if (!this->isFlushed) {
        std::fflush(this->file);
        this->isFlushed = true;
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDiskStorage<char>;
template class IDiskStorage<uchar>;
