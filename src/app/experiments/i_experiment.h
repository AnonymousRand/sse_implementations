#pragma once

#include <concepts>

#include "schemes/interfaces/i_sse.h"


namespace app::experiments {


template <IsSse Sse>
class IExperiment {
public:
    virtual void printHeader() const = 0;
    virtual void run(Sse* sse) const = 0;
};


} // namespace `app::experiments`
