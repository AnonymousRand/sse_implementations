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
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;
};

template <template<class ...> class T, class DbDoc, class DbKw> concept ISse_ = requires(T<DbDoc, DbKw> t) {
    []<class X, class Y>(ISse<X, Y>&){}(t);
};

////////////////////////////////////////////////////////////////////////////////
// `IDsse`
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// `ISdaUnderly`
////////////////////////////////////////////////////////////////////////////////

template <IDbDoc_ DbDoc, class DbKw>
class ISdaUnderly : public ISse<DbDoc, DbKw> {
    public:
        virtual std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const = 0;
        virtual Db<DbDoc, DbKw> getDb() const = 0;
        virtual bool isEmpty() const = 0;
};

template <class T, class DbDoc, class DbKw> concept ISdaUnderly_ = std::derived_from<T, ISdaUnderly<DbDoc, DbKw>>;
