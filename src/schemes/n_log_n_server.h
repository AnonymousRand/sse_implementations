#pragma once

#include "utils/utils.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class NLogNServer = {
    public:
        ~NLogNServer();

        void clear();
        void addEncIndLvl(EncInd* encIndLvl);
        void initDbKwListSizeDict(long size);
        void writeToEncInd(long lvl, ulong pos, const EncIndEntry& entry);
        std::vector<EncIndVal> getBucket(long lvl, ulong pos, const ustring& label) const;

        EncInd* getEncIndLvl(long lvl) const;

    protected:
        std::vector<EncInd*> encIndLvls;

    private:
        // stuff to not share with Log-SRC-i*
        EncInd* dbKwListSizeDict = new EncInd();
};
