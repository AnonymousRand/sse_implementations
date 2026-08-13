#include "utils/disk_storage.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
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


IDiskStorage::IDiskStorage() {
    this->initFile();
}


IDiskStorage::~IDiskStorage() {
    this->clearFile();
}


std::string IDiskStorage::genFilename() const {
    // avoid naming clashes by generating a random 8 byte (16 char) hex string
    uint64_t randomHex = ::dist(utils::RNG);
    std::string randomHexStr = std::format("{:016x}", randomHex);
    return this->FILENAME_PREFIX() + randomHexStr + ".dat";
}


void IDiskStorage::initFile() {
    this->filename = this->genFilename();
    FILE* fileTmp = std::fopen(this->filename.c_str(), "r");
    // while file exists (or any other error occurs on open), create new random filename
    while (fileTmp != nullptr) {
        std::fclose(fileTmp);
        this->filename = this->genFilename();
        fileTmp = std::fopen(this->filename.c_str(), "r");
    }
    std::fclose(fileTmp);
    
    this->file = std::fopen(this->filename.c_str(), "wb+");
    if (this->file == nullptr) {
        std::cerr << "Error: IDiskStorage::initFile(): error opening file" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void IDiskStorage::clearFile() {
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
