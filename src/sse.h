#pragma once


#include "util/enc_ind.h"


/******************************************************************************/
/* `ISse`                                                                     */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class ISse {
    protected:
        int secParam;

    public:
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        
        /**
         * Params:
         *     - `shouldCleanUpResults`: whether to filter out deleted docs or not
         *     - `isNaive`: whether to search each point in `query` individually, or the entire range in one go
         *       (i.e. `query` itself must be in the db), e.g. as underlying for Log-SRC.
         */
        virtual std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const = 0;

        /**
         * Free memory and clear the db/index, without fully destroying this object as the destructor does
         * (so we can still call `setup()` again with the same object, perhaps with a different db).
         * 
         * Notes:
         *     - Should be idempotent and safe to call without `setup()` first as well.
         */
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


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};


/******************************************************************************/
/* `ISdaUnderly`                                                              */
/******************************************************************************/


template <IDbDoc_ DbDoc, class DbKw>
class ISdaUnderly {
    public:
        virtual Db<DbDoc, DbKw> getDb() const = 0;
        virtual bool isEmpty() const = 0;
};


template <class T>
concept ISdaUnderly_ = requires(T t) {
    []<class ... Args>(ISdaUnderly<Args ...>&){}(t);
};
