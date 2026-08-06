#pragma once

#include <vector>

#include "utils/enc_ind.h"
#include "utils/utils.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
class PiBasServer {
    public:
        ~PiBasServer();

        void clear();
        void setEncInd(EncInd* encInd);
        std::vector<EncIndVal> search(const ustring& queryToken) const;

        bool getEncIndVal(ulong pos, EncIndVal& ret) const;

    private:
        EncInd* encInd;
};
