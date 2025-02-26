#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// `ISse`
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class ISse {
    protected:
        int secParam;

    public:
        bool isEmpty = false;

        // API functions
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;
};

////////////////////////////////////////////////////////////////////////////////
// `IDsse`
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        // API functions
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// `IUnderly`
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class IUnderly : public ISse<DbDoc, DbKw> {
    public:
        virtual Db<DbDoc, DbKw> getDb() const = 0;
};

template <template<class ...> class T, class T1, class T2> concept IUnderly_ = requires(T<T1, T2> t) {
    []<class X, class Y>(IUnderly<X, Y>&){}(t);
};
