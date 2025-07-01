#pragma once


#include "log_src_i.h" 
#include "pi_bas.h" 


/******************************************************************************/
/* `ILogSrcILocUnderly`                                                       */

/******************************************************************************/


template <class DbKw>
class ILogSrcILocUnderly {
    protected:
        IEncIndLoc<DbKw>* encInd = nullptr;
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
template <template <class ...> class Underly> requires IsLogSrcILocUnderly<Underly<Doc, Kw>>
class LogSrcILoc : public LogSrcIBase<Underly> {
    public:
        LogSrcILoc();
        LogSrcILoc(EncIndType encIndType);

        void setup(int secParam, const Db<Doc, Kw>& db) override;
};


/******************************************************************************/
/* `PiBasLoc`                                                                 */
/******************************************************************************/


template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
class PiBasLoc : public ILogSrcILocUnderly<DbKw>, public PiBasBase<DbDoc, DbKw> {
    private:
        std::unordered_map<Range<DbKw>, long> kwResCounts;
        long leafCount;
        long minDbKw;

        std::vector<DbDoc> searchBase(const Range<DbKw>& query) const override;

    public:
        PiBasLoc() = default;
        PiBasLoc(EncIndType encIndType);
        ~PiBasLoc();

        void setup(int secParam, const Db<DbDoc, DbKw>& db) override;

        void clear() override;
        void setEncIndType(EncIndType encIndType) override;
};
