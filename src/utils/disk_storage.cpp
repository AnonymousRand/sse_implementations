#include "utils/disk_storage.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <random>
#include <string>
#include <utility>

#include "utils/random.h"
#include "utils/types.h"


namespace {


std::uniform_int_distribution<ubigint> dist;


} // anonymous namespace


//==============================================================================
// `IDiskStorage`
//==============================================================================


//------------------------------------------------------------------------------
// constructors/destructors


IDiskStorage::~IDiskStorage() {
    this->clear();
}


//------------------------------------------------------------------------------
// copy/move


void IDiskStorage::copyFrom(const IDiskStorage& other) {
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
        std::cerr << "Error: IDiskStorage::copyFrom(): error copying file: " << e.what()
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // open the file we just copied
    // (we use `a` instead of `w` mode here to not overwrite the file we just copied)
    this->file = std::fopen(this->filename.c_str(), "ab+");
    if (this->file == nullptr) {
        std::cerr << "Error: IDiskStorage::copyFrom(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void IDiskStorage::moveFrom(IDiskStorage&& other) noexcept {
    // important: set all fields in `other` that have non-default destruction/should not be
    // double-freed to a null value, so that its destructor doesn't try to delete the same resource
    // (e.g.  pointer or filename) that `this`'s fields now point to when it goes out of scope
    this->file = other.file;
    other.file = nullptr;

    this->filename = other.filename;
    other.filename = "";

    //std::cerr << "===== this now has " << this << "; " << this->filename << ", " << this->file << "; other now has " << &other << "; " << other.filename << ", " << other.file << std::endl;

    this->isFlushed = other.isFlushed;
}


// copy assignment operator
IDiskStorage& IDiskStorage::operator =(const IDiskStorage& other) {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->copyFrom(other);
    }
    return *this;
}


// move constructor
IDiskStorage::IDiskStorage(IDiskStorage&& other) noexcept {
    this->moveFrom(std::move(other));
}


// move assignment operator
IDiskStorage& IDiskStorage::operator =(IDiskStorage&& other) noexcept {
    //std::cerr << "===== copying to: " << this << "; " << this->filename << ", " << this->file << " from " << &other << "; " << other.filename << ", " << other.file << std::endl;
    // important self-assignment safety check!
    if (this != &other) {
        //std::cerr << "===== clearing " << this->filename << ", " << this->file << std::endl;
        this->clear();
        this->moveFrom(std::move(other));
    }
    return *this;
}


//------------------------------------------------------------------------------
// interface


void IDiskStorage::init() {
    this->clear();

    // first make sure base directory exists
    try {
        std::filesystem::create_directories(this->FILE_DIR());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: IDiskStorage::init(): error creating path " << this->FILE_DIR()
                  << ": " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->filename = this->genFilename();

    // while file exists (or any other error occurs on open), create new random filename
    while (std::filesystem::exists(this->filename)) {
        this->filename = this->genFilename();
    }
    
    this->file = std::fopen(this->filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error: IDiskStorage::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void IDiskStorage::clear() {
    //std::cerr << "+++++ clear: " << this << "; " << this->filename << ", " << this->file << std::endl;
    // close file descriptors
    if (this->file != nullptr) {
        //std::cerr << "+++++ clear " << this->file << std::endl;
        std::fclose(this->file);
        this->file = nullptr;
        //std::cerr << "+++++ file is now " << this->file << std::endl;
    }

    // delete file from disk
    if (this->filename != "") {
        //std::cerr << "+++++ deleting " << this->filename << std::endl;
        std::remove(this->filename.c_str());
        this->filename = "";
    }
}


//--------------------------------------------------------------------------
// helpers


std::string IDiskStorage::genFilename() const {
    // avoid naming clashes by generating a random 8 byte (16 char) hex string
    ubigint randomHex = ::dist(utils::RNG);
    std::string randomHexStr = std::format("{:016x}", randomHex);
    return std::format("{}/{}{}.dat", this->FILE_DIR(), this->FILENAME_PREFIX(), randomHexStr);
}
