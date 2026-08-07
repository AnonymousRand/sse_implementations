#pragma once

#include "schemes/log_src_i.h" 
#include "schemes/n_log_n.h" 


//------------------------------------------------------------------------------
// `LogSrcIStarUnderly`
//------------------------------------------------------------------------------


namespace underly {


// this is specifcally designed to avoid using NLogN as a black box for Log-SRC-i* (the same way one may use PiBas)
// which blows up the storage unnecessarily, as observed in the TODS'18 paper (Section 7.1)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class LogSrcIStarUnderly : public NLogN<DbDoc, DbKw> {
    public:
        using NLogN<DbDoc, DbKw>::NLogN;

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

    private:
        int64_t leafCount;

        //----------------------------------------------------------------------
        // `IStaticPointSse`

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

        //----------------------------------------------------------------------
        // other

        int64_t computeNumLvls() const override;
        int64_t computeBcktCountOnLvl(int64_t lvlNum) const override;
};


} // namespace `underly`


//------------------------------------------------------------------------------
// `LogSrcIStar`
//------------------------------------------------------------------------------


class LogSrcIStar : public LogSrcIBase<underly::LogSrcIStarUnderly> {
    public:
        using LogSrcIBase<underly::LogSrcIStarUnderly>::LogSrcIBase;

        //----------------------------------------------------------------------
        // `ISse`

        /**
         * Preconditions:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};
