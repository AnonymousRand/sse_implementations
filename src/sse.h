#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// ISse
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class ISse {
    public:
        bool isIndEmpty = false;

        // API functions
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;
};

template <template<class ...> class T, class T1, class T2> concept ISseDeriv = requires(T<T1, T2> t) {
    []<class X, class Y>(ISse<X, Y>&){}(t);
};

////////////////////////////////////////////////////////////////////////////////
// IDsse
////////////////////////////////////////////////////////////////////////////////

template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        // API functions
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
        // todo note in readme that deletion tuples are same id and ke but diff operation
};
