#pragma once

#include <concepts>

#include "schemes/interfaces/sse.h"

#include "types/basic_types.h"
#include "types/tuple.h"


// (note: no virtual inheritance here as otherwise things extending `IDsse` will skip over `IDsse`'s
// constructors, leading to the explicit constructor below with `useShortcutSetup` to not be seen)
template <IsDbTuple DbTuple = Tuple<>>
class IDsse : public ISse<DbTuple> {
protected:
    using DbKw = typename ISse<DbTuple>::DbKw;

public:
    IDsse(bool useShortcutSetup) : useShortcutSetup(useShortcutSetup) {}

    //--------------------------------------------------------------------------
    // interface

    virtual void update(const DbTuple& newTuple) = 0;

    //--------------------------------------------------------------------------
    // shared code

    // handle clearing of this class' member variables
    void clear() override {
        this->updateCount = 0;
    }

protected:
    /**
     * whether to use the shortcut `setup()` (non-shortcut is calling `update()`
     * for each item in `db`). the shortcut should only be used to speed up `setup()`
     * for experimental evaluation of searches.
     */
    bool useShortcutSetup = false;

    bigint updateCount = 0;
};


template <class T>
concept IsDsse = requires(T t) {
    []<class ... Args>(IDsse<Args ...>&){}(t);
};
