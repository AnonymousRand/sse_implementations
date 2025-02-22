#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// ISse
////////////////////////////////////////////////////////////////////////////////

template <class DbDoc = Doc, class DbKw = Kw>
class ISse {
    public:
        virtual void setup(int secParam, const Db<DbDoc, DbKw>& db) = 0;
        // todo does compiler optimization bypass this? use out parameter `results` instead of returning it to avoid copying large amounts of data
        virtual std::vector<DbDoc> search(const Range<DbKw>& query) const = 0;
};

////////////////////////////////////////////////////////////////////////////////
// IDsse
////////////////////////////////////////////////////////////////////////////////

template <class DbDoc = Doc, class DbKw = Kw>
class IDsse : public ISse<DbDoc, DbKw> {
    public:
        // API functions
        //virtual void update();
};
