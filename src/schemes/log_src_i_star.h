#pragma once


#include "log_src_i.h" 
#include "pibas.h" 
#include "utils/enc_ind.h"


//==============================================================================
// `LogSrcIStar`
//==============================================================================


// this construction is a separate file since it requires rather specific code (e.g. padding) for any underlying schemes
// and it also doesn't make sense to instantiate `LogSrcIStarUnderly` by itself since it is specific to `LogSrcIStar`
template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcIStar : public LogSrcIBase<Underly> {
    public:
        //----------------------------------------------------------------------
        // `ISse`

        /**
         * Preconditions:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};


//==============================================================================
// `LogSrcIStarUnderly`
//==============================================================================


namespace underly {


/*
todo brainstorming: for log-src-i* underly, just override setup() with now each level
storing the custom amount of buckets/entries based on # of tdag nodes on that level 
and for the size param use the number of leaves
also remove padding right
*/

// this is specifcally designed to avoid using NlogN as a black box for Log-SRC-i* (the same way one may use Pibas)
// which blows up the storage unnecessarily, as observed in the TODS'18 paper, Section 7.1
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class LogSrcIStarUnderly : public Nlogn<DbDoc, DbKw> {
    public:
        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

    private:
        long leafCount;
};


} // namespace `underly`
