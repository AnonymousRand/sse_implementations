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


template <IsDbTuple DbTuple = Tuple<>>
Db<DbTuple>::Db() {
    this->initFile();
}


template <IsDbTuple DbTuple = Tuple<>>
Db<DbTuple>::Db(const Db& db) {
    this->filename = this->genFilename();
    this->size = db->size;

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


template <IsDbTuple DbTuple = Tuple<>>
Db<DbTuple>::~Db() {
    this->clear();
}
