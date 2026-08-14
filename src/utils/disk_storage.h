#pragma once

#include <cstdio>
#include <string>


class IDiskStorage {
public:
    //--------------------------------------------------------------------------
    // constructors/destructors

    IDiskStorage();

    /**
     * move assignment operator that allows cheap moving instead of expensive copying of
     * `IDiskStorage` rvalues when they are assigned to an existing variable, e.g. `*this = ...`.
     * (in particular, this is responsible for freeing the old destination's resources
     * that come from this class.)
     */
    // summary of special constructors/assignment operators:
    // =======================================================================
    // ‖ assigning \ assigning ‖    new object    |     existing object      ‖
    // ‖   from:    \    to:   ‖                  |                          ‖
    // =======================================================================
    // ‖        lvalue         ‖ copy constructor | copy assignment operator ‖
    // ‖                       ‖ `T obj2 = obj1`  |      `obj2 = obj1`       ‖
    // ‖---------------------------------------------------------------------‖
    // ‖        rvalue         ‖ move constructor | move assignment operator ‖
    // ‖                       ‖  `T obj2 = f()`  |      `obj2 = f()`        ‖
    // =======================================================================
    // note that starting C++17, move constructors (bottom left) in the case where the rvalue
    // is a function call are replaced by mandatory "copy elision", which also avoids a copy
    IDiskStorage& operator =(IDiskStorage&& other) noexcept;

    ~IDiskStorage();

    //--------------------------------------------------------------------------
    // interface

    virtual void clear();

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
    void initFile();
};
