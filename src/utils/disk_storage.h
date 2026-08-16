#pragma once

#include <cstdio>
#include <string>


class IDiskStorage {
public:
    // IMPORTANT: constructors/destructors/related methods here should be responsible for
    // the members *defined by this class*!

    //--------------------------------------------------------------------------
    // constructors/destructors

    // default constructor forced so that children can use a default constructor too
    IDiskStorage() = default;

    ~IDiskStorage();

    //--------------------------------------------------------------------------
    // rule of five

    // copy constructor
    IDiskStorage(const IDiskStorage& other) = default;

    // copy assignment operator
    IDiskStorage& operator =(const IDiskStorage& other) = default;

    // move constructor
    IDiskStorage(IDiskStorage&& other) noexcept = default;

    // move assignment operator
    // (allows cheap moving instead of expensive copying of `IDiskStorage` rvalues when they are
    // assigned to an existing variable, e.g. `*this = ...`)
    IDiskStorage& operator =(IDiskStorage&& other) noexcept;

    //--------------------------------------------------------------------------
    // interface

    virtual void init();
    virtual void clear();

    //--------------------------------------------------------------------------
    // debugging

    FILE* getFile() const {
        return this->file;
    }

    std::string getFilename() const {
        return this->filename;
    }

protected:
    FILE* file = nullptr;
    std::string filename = "";

    //--------------------------------------------------------------------------
    // methods to implement

    virtual constexpr std::string FILE_DIR() const = 0;
    virtual constexpr std::string FILENAME_PREFIX() const = 0;

    //--------------------------------------------------------------------------
    // helpers

    std::string genFilename() const;
};
