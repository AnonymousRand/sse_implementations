#pragma once


#include "log_src_i.h" 
#include "pibas.h" 
#include "utils/enc_ind.h"


//==============================================================================
// `LogSrcIStar`
//==============================================================================


// this construction is a separate file since it requires rather specific code (e.g. padding) for each underlying scheme
// and it also doesn't make sense to instantiate `PibasLoc` by itself since its encrypted index is specific for TDAGs
template <template <class ...> class Underly> requires IsSse<Underly<Doc<>, Kw>>
class LogSrcIStar : public LogSrcIBase<Underly> {
    public:
        //----------------------------------------------------------------------
        // `ISse`

        /**
         * Precondition:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};


//==============================================================================
// `PibasLoc`
//==============================================================================


namespace underly {


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class PibasLoc : public PibasBase<DbDoc, DbKw> {
    public:
        ~PibasLoc();

        //----------------------------------------------------------------------
        // `ISse`

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

    private:
        EncIndLoc<DbKw>* encInd = new EncIndLoc<DbKw>();
        std::unordered_map<Range<DbKw>, long> dbKwListSizes;
        long leafCount;
        long minDbKw;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
        bool readEncIndValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const override;
};


} // namespace `underly`
