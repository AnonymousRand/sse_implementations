#include "utils/disk_storage.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <random>
#include <string>

#include "utils/random.h"


namespace {


std::uniform_int_distribution<uint64_t> dist;


} // anonymous namespace


//==============================================================================
// `IDiskStorage`
//==============================================================================


//------------------------------------------------------------------------------
// constructors/destructors


IDiskStorage& IDiskStorage::operator =(IDiskStorage&& other) noexcept {
    //std::cerr << "move assignment constructor" << std::endl;
    // important self-assignment safety check!
    if (this != &other) {
        this->clear();
        //std::cerr << "move assignment constructor cleared `this`" << std::endl;
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
    
    // note: use `a` instead of `w` mode since there can be times (e.g. `fopen()`ing in a copy
    // constructor after using `std::filesystem::copy_file()`) where we want to `fopen()` an
    // existing file and NOT overwrite it
    this->file = std::fopen(this->filename.c_str(), "ab+");
    if (this->file == nullptr) {
        std::cerr << "Error: IDiskStorage::init(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    //std::cerr << "file opened at " << this->filename.c_str() << ", pointer " << this->file << std::endl;
}


void IDiskStorage::clear() {
    // close file descriptors
    if (this->file != nullptr) {
        std::fclose(this->file);
        this->file = nullptr;
    }

    // delete file from disk
    if (this->filename != "") {
        //std::cerr << "clear() called, clearing " << this->filename << std::endl;
        std::remove(this->filename.c_str());
        this->filename = "";
    }
}


//--------------------------------------------------------------------------
// helpers


std::string IDiskStorage::genFilename() const {
    // avoid naming clashes by generating a random 8 byte (16 char) hex string
    uint64_t randomHex = ::dist(utils::RNG);
    std::string randomHexStr = std::format("{:016x}", randomHex);
    //std::cerr << "genFilename called, output is " << randomHexStr << std::endl;
    return std::format("{}/{}{}.dat", this->FILE_DIR(), this->FILENAME_PREFIX(), randomHexStr);
}
