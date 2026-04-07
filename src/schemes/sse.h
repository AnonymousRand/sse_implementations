// definitions of methods (i.e. shared code) are given inline in this file
// to avoid all the explicit template instantiation needed if using a `.cpp` file


#pragma once


#include "utils/cryptography.h"
#include "utils/utils.h"


//==============================================================================
// `ISse`
//==============================================================================


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISse {
    public:
        //----------------------------------------------------------------------
        // methods to implement

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


//==============================================================================
// `IStaticPointSse`
//==============================================================================


// subclasses of this include `Pibas`, `Nlogn`, and `LogSrcIStarUnderly`
// provide shared code for `search()` (depending on `searchBase()`)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class IStaticPointSse : public virtual ISse<DbDoc, DbKw> {
    public:
        //----------------------------------------------------------------------
        // shared code

        inline std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override {
            std::vector<DbDoc> allResults;

            if (isNaive) {
                // naive, insecure range search: just individually query every point in range
                for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
                    std::vector<DbDoc> results = this->searchBase(Range {dbKw, dbKw});
                    allResults.insert(allResults.end(), results.begin(), results.end());
                }
            } else {
                // search entire range in one go (i.e. `query` itself must be in the db), e.g. as underlying for Log-SRC
                allResults = this->searchBase(query);
            }

            std::cout << "results before clean up: " << std::endl;
            for (auto result : allResults) {
                std::cout << result << std::endl;
            }
            std::cout << std::endl;
            if (shouldCleanUpResults) {
                cleanUpResults(allResults);
            }
            return allResults;
        }

        // handle clearing of `prfKey` and `encKey` member variables belonging to this interface
        inline void clear() override {
            this->prfKey = toUstr("");
            this->encKey = toUstr("");
        }

    protected:
        ustring prfKey;
        ustring encKey;

        //----------------------------------------------------------------------
        // methods to implement

        virtual std::vector<DbDoc> searchBase(const Range<DbKw>& query) const = 0;
        
        //----------------------------------------------------------------------
        // shared code

        /**
         * Helper function to decrypt `encIndVal`.
         */
        inline DbDoc decryptEncIndVal(const EncIndVal& encIndVal) const {
            ustring encDbDoc = encIndVal.first;
            ustring iv = encIndVal.second;
            ustring decDbDoc = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbDoc, iv);
            return DbDoc::fromUstr(decDbDoc);
        }
};


//==============================================================================
// `IDsse`
//==============================================================================


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class IDsse : public virtual ISse<DbDoc, DbKw> {
    public:
        //----------------------------------------------------------------------
        // methods to implement

        virtual void update(const DbEntry<DbDoc, DbKw>& newEntry) = 0;
};


//==============================================================================
// `ISdaUnderlySse`
//==============================================================================


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class ISdaUnderlySse : public virtual ISse<DbDoc, DbKw> {
    public:
        //----------------------------------------------------------------------
        // methods to implement

        /**
         * Append the `db` most recently passed to `setup()` (without any replications/padding/processing) to `ret`.
         */
        virtual void getDb(Db<DbDoc, DbKw>& ret) const = 0;

        //----------------------------------------------------------------------
        // shared code

        // handle clearing of `size` member variable belonging to this interface
        inline void clear() override {
            this->size = 0;
        }

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
