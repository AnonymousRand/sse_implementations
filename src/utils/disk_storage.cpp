#include "utils/disk_storage.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <random>
#include <string>

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


// copy constructor
IDiskStorage::IDiskStorage(const IDiskStorage& other) {
    std::cerr << "----- IDiskStorage copy constructor from " << other.filename << std::endl;
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
        std::cerr << "Error: IDiskStorage::IDiskStorage(const DbDisk&): error copying file: "
                  << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // open the file we just copied
    // (we use `a` instead of `w` mode here to not overwrite the file we just copied)
    this->file = std::fopen(this->filename.c_str(), "ab+");
    if (this->file == nullptr) {
        std::cerr << "Error: IDiskStorage::IDiskStorage(const DbDisk&): error opening file"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cerr << "----- this->filename is now " << this->filename << std::endl;
}


// move assignment operator
IDiskStorage& IDiskStorage::operator =(IDiskStorage&& other) noexcept {
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        this->file = other.file;
        // important: set all fields in `other` that have non-default destruction/clear above
        // to a null value so that its destructor doesn't try to delete the same resource (e.g.
        // pointer or filename) that `this`'s fields now point to when it goes out of scope here
        other.file = nullptr;
        this->filename = other.filename;
        other.filename = "";
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
    // close file descriptors
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }

    // delete file from disk
    if (this->filename != "") {
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
