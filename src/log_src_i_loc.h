#pragma once


#include "log_src_i.h" 
#include "pi_bas.h" 
#include "utils/enc_ind.h"


/******************************************************************************/
/* `ILogSrcILocUnderly`                                                       */
/******************************************************************************/


template <class DbKw>
class ILogSrcILocUnderly {
    protected:
        EncIndLoc<DbKw>* encInd = new EncIndLoc<DbKw>;
};


template <class T>
concept IsLogSrcILocUnderly = requires(T t) {
    []<class ... Args>(ILogSrcILocUnderly<Args ...>&){}(t);
};


/******************************************************************************/
/* `LogSrcILoc`                                                               */
/******************************************************************************/


// this construction is a separate file since it requires rather specific code (e.g. padding) for each underlying scheme
// and it also doesn't make sense to instantiate `PiBasLoc` by itself since its encrypted index is specific for TDAGs
template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc<>, Kw>>
class LogSrcILoc : public LogSrcIBase<Underly> {
    public:
        LogSrcILoc();

        // `ISse`
        /**
         * Precondition:
         *     - Entries in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
         *     - Entries in `db` cannot have keyword equal to `DUMMY`.
         */
        void setup(int secParam, const Db<Doc<>, Kw>& db) override;
};


/******************************************************************************/
/* `PiBasLoc`                                                                 */
/******************************************************************************/


namespace underly {

template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class PiBasLoc : public ILogSrcILocUnderly<DbKw>, public PiBasBase<DbDoc, DbKw> {
    public:
        ~PiBasLoc();

        // `ISse`
        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;
        void clear() override;

    private:
        std::unordered_map<Range<DbKw>, long> dbKwCounts;
        long leafCount;
        long minDbKw;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;
};

} // namespace `underly`
