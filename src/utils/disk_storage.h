#pragma once

#include <cstdio>
#include <string>


class IDiskStorage {
protected:
    FILE* file = nullptr;
    std::string filename = "";

    virtual const std::string FILENAME_PREFIX() const = 0;

    IDiskStorage();
    ~IDiskStorage();

    std::string genFilename() const;
    void initFile();
    void clearFile();
};
