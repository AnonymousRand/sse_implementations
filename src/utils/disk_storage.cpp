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


IDiskStorage::~IDiskStorage() {
    this->clear();
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
