#pragma once

#include <concepts>
#include <memory>

#include "schemes/interfaces/sse.h"

#include "utils/db.h"
#include "utils/sse_utils.h"


// forward declare instead of include to avoid a circular include with `benchmark.h`
struct Benchmark;


// (note: no virtual inheritance here as otherwise things extending `IDsse` will skip over `IDsse`'s
// constructors, leading to the explicit constructor below with `useShortcutSetup` to not be seen)
template <class DbRecord = Record<>, class DbKw = Kw> requires IsValidDbParams<DbRecord, DbKw>
class IDsse : public ISse<DbRecord, DbKw> {
public:
    using ISse<DbRecord, DbKw>::ISse;

    IDsse(std::shared_ptr<Benchmark> benchmark, bool useShortcutSetup) :
        ISse<DbRecord, DbKw>(benchmark), useShortcutSetup(useShortcutSetup) {}

    //----------------------------------------------------------------------
    // methods to implement

    virtual void update(const DbRecord<DbRecord, DbKw>& newRecord) = 0;

protected:
    /**
     * whether to use the shortcut `setup()` (non-shortcut is calling `update()`
     * for each item in `db`). the shortcut should only be used to speed up `setup()`
     * for experimental evaluation of searches.
     */
    bool useShortcutSetup = false;
};


template <class T>
concept IsDsse = requires(T t) {
    []<class ... Args>(IDsse<Args ...>&){}(t);
};
