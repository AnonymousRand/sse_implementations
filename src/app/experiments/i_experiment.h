#pragma once

#include <concepts>

#include "schemes/interfaces/sse.h"


namespace app::experiments {


template <IsSse Sse>
class IExperiment {
public:
    virtual void printHeader() const = 0;
    virtual void run(Sse* sse, bool shouldBenchmark) const = 0;
};


} // namespace `app::experiments`
