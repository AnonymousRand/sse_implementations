#pragma once


#include "util/enc_ind.h"
#include "util/util.h"


/******************************************************************************/
/* `ISse`                                                                     */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class ISse {
    protected:
        int secParam;

    public:
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;

        // `clear()` should free memory of all unneeded instance variables, but not fully delete this object
        // (as the destructor does) so we can still call `setup()` again with the same object, perhaps witha different db
        virtual void clear() = 0;
        virtual void setEncIndType(EncIndType encIndType) = 0;
};


template <class T>
concept ISse_ = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};


/******************************************************************************/
/* `IDsse`                                                                    */
/******************************************************************************/


template <IDbDoc_ DbDoc = Kw, class DbKw = Kw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};


/******************************************************************************/
/* `ISdaUnderlySse`                                                           */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
class ISdaUnderlySse : public ISse<DbDoc, DbKw> {
    public:
        virtual std::vector<DbDoc> searchWithoutRemovingDels(const Range<DbKw>& query) const = 0;
        virtual Db<DbDoc, DbKw> getDb() const = 0;
        virtual bool isEmpty() const = 0;
};


template <class T>
concept ISdaUnderlySse_ = requires(T t) {
    []<class ... Args>(ISdaUnderlySse<Args ...>&){}(t);
};
