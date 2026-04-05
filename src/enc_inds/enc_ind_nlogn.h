# pragma once


#include "enc_ind_base.h"
#include "enc_ind_pibas.h"


// this manages a collection of Pibas encrypted indexes, one for each level
template <class DocDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
class EncIndNlogn {
    public:
        void init(long size) override;
        void clear() override;
        void find(
            const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const;
        void write(
            const ustring& key, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
        );

        /**
         * Return numerical level & position at which to place entry in index (in a locality-preserving manner).
         */
        std::pair<ulong, ulong> map(
            const ustring& prfKey, const ustring& encKey, DbDoc dbDoc, const Range<DbKw>& dbKwRange, long dbKwCounter,
            EncIndEntry& retEncIndEntry
        ) const;

    private:
        std::vector<EncIndPibas<DbDoc, DbKw>*> levels;
};
