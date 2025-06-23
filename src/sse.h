#pragma once

#include "util/enc_ind.h"
#include "util/util.h"

/******************************************************************************/
/* `ISse`                                                                     */
/******************************************************************************/

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
class ISse {
    protected:
        int secParam;

    public:
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;
};

template <class T>
concept ISse_ = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};

/******************************************************************************/
/* `IDsse`                                                                    */
/******************************************************************************/

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
class IDsse : public ISse<EncInd, DbDoc, DbKw> {
    public:
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};

/******************************************************************************/
/* `ISdaUnderly`                                                              */
/******************************************************************************/

template <IEncInd_ EncInd, IDbDoc_ DbDoc, class DbKw>
class ISdaUnderly : public ISse<EncInd, DbDoc, DbKw> {
    public:
        virtual std::vector<DbDoc> searchWithoutHandlingDels(const Range<DbKw>& query) const = 0;
        virtual Db<DbDoc, DbKw> getDb() const = 0;
        virtual bool isEmpty() const = 0;
};

template <class T>
concept ISdaUnderly_ = requires(T t) {
    []<class ... Args>(ISdaUnderly<Args ...>&){}(t);
};
