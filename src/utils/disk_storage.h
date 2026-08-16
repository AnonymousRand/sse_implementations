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
    // (IMPORTANT: children should call `IDiskStorage::init()` to properly initialize members,
    // whether it's in its own constructor or `init()` function. the `IDiskStorage` constructors
    // cannot perform the logic in `init()` automatically as it calls pure virtual functions,
    // which cannot be done in the base class constructors.)
    IDiskStorage() = default;

    ~IDiskStorage();

    //--------------------------------------------------------------------------
    // copy/move

protected:
    // unified functions for copying and moving
    void copyFrom(const IDiskStorage& other);
    void moveFrom(IDiskStorage&& other) noexcept;

public:
    // copy constructor
    // (deleted as children MUST call `IDiskStorage::copy()` in their copy constructors instead,
    // for the same reason as `init()`)
    IDiskStorage(const IDiskStorage& other) = delete;

    // copy assignment operator
    IDiskStorage& operator =(const IDiskStorage& other);

    // move constructor
    IDiskStorage(IDiskStorage&& other) noexcept;

    // move assignment operator
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
    // (`mutable` allows this to be modified in `const` contexts still)
    mutable bool isFlushed = true;

    //--------------------------------------------------------------------------
    // methods to implement

    virtual constexpr std::string FILE_DIR() const = 0;
    virtual constexpr std::string FILENAME_PREFIX() const = 0;

    //--------------------------------------------------------------------------
    // helpers

    std::string genFilename() const;
};
