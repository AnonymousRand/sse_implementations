#pragma once

#include "util/util.h"

////////////////////////////////////////////////////////////////////////////////
// ISse
////////////////////////////////////////////////////////////////////////////////

template <typename DbDocType>
class ISse {
    public:
        // API functions
        virtual void setup(int secParam, const Db<DbDocType>& db) = 0;
        // todo does compiler optimization bypass this? use out parameter `results` instead of returning it to avoid copying large amounts of data
        virtual std::vector<DbDocType> search(const KwRange& range) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// IRangeSse
////////////////////////////////////////////////////////////////////////////////

template <typename Underlying>
class IRangeSse {
    protected:
        const Underlying& underlying;

    public:
        IRangeSse(const Underlying& underlying);
};
