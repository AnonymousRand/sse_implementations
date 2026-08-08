#pragma once

#include <concepts>
#include <memory>

#include "schemes/interfaces/sse.h"

#include "utils/doc.h"
#include "utils/sse_utils.h"


// (note: no virtual inheritance here as otherwise things extending `IDsse` will skip over `IDsse`'s
// constructors, leading to the explicit constructor below with `useShortcutSetup` to not be seen)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class IDsse : public ISse<DbDoc, DbKw> {
public:
    using ISse<DbDoc, DbKw>::ISse;
    IDsse(std::shared_ptr<Benchmark> benchmark, bool useShortcutSetup) :
            ISse<DbDoc, DbKw>(benchmark), useShortcutSetup(useShortcutSetup) {}

    //----------------------------------------------------------------------
    // methods to implement

    virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;

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
