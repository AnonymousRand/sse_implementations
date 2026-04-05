# pragma once


#include "enc_ind_base.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
class EncIndPibas : public EncIndBase {
    public:
        void init(long size) override;
        void clear() override;
        bool find(
            const ustring& prfKey, const ustring& encKey, const Range<DbKw>& query, long dbKwCounter,
            std::pair<ustring, ustring>& ret
        ) const;
        void write(
            const ustring& prfKey, const ustring& encKey, DbDoc dbDoc, const Range<DbKw>& dbKwRange, long dbKwCounter,
            const std::pair<ustring, ustring>& val
        );
 
    protected:
        void writeToPos(ulong pos, const EncIndEntry& encIndEntry, bool flushImmediately = false) override;
};
