#pragma once

#include <cstdio>
#include <string>


class IDiskStorage {
public:
    IDiskStorage();
    ~IDiskStorage();

    virtual void clear();

protected:
    FILE* file = nullptr;
    std::string filename = "";

    virtual constexpr std::string FILE_DIR() const = 0;
    virtual constexpr std::string FILENAME_PREFIX() const = 0;

    //--------------------------------------------------------------------------
    // helpers

    std::string genFilename() const;
    void initFile();
};
