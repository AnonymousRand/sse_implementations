#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// ISse
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc>
class ISse {
    public:
        // API functions
        virtual void setup(int secParam, const Db<DbDoc>& db) = 0;
        // todo does compiler optimization bypass this? use out parameter `results` instead of returning it to avoid copying large amounts of data
        virtual std::vector<DbDoc> search(const KwRange& range) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// IDsse
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc>
class IDsse : public ISse<DbDoc> {
    public:
        // API functions
        virtual void update();
};
