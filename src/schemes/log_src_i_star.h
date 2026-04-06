#pragma once


#include "log_src_i.h" 
#include "pibas.h" 
#include "utils/enc_ind.h"


//==============================================================================
// `LogSrcIStarUnderly`
//==============================================================================


namespace underly {


// this is specifcally designed to avoid using NlogN as a black box for Log-SRC-i* (the same way one may use Pibas)
// which blows up the storage unnecessarily, as observed in the TODS'18 paper (Section 7.1)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class LogSrcIStarUnderly : public Nlogn<DbDoc, DbKw> {
    public:
        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
};


} // namespace `underly`


//==============================================================================
// `LogSrcIStar`
//==============================================================================


class LogSrcIStar : public LogSrcIBase<underly::LogSrcIStarUnderly> {
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
