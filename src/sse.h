#pragma once


#include "utils/utils.h"


/******************************************************************************/
/* `ISse`                                                                     */
/******************************************************************************/


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISse {
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

    protected:
        int secParam;
};


template <class T>
concept IsSse = requires(T t) {
    []<class ... Args>(ISse<Args ...>&){}(t);
};


/******************************************************************************/
/* `IDsse`                                                                    */
/******************************************************************************/


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class IDsse : public virtual ISse<DbDoc, DbKw> {
    public:
        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};


/******************************************************************************/
/* `ISdaUnderlySse`                                                           */
/******************************************************************************/


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISdaUnderlySse : public virtual ISse<DbDoc, DbKw> {
    public:
        virtual Db<DbDoc, DbKw> getDb() const = 0;
        inline long getSize() const {
            return this->size;
        }

    protected:
        long size;
};


template <class T>
concept IsSdaUnderlySse = requires(T t) {
    []<class ... Args>(ISdaUnderlySse<Args ...>&){}(t);
};
