#pragma once

#include <concepts>
#include <memory>

#include "schemes/interfaces/i_dsse.h"
#include "schemes/interfaces/i_sse.h"

#include "utils/benchmark.h"


namespace app {


template <class Sse> requires IsSse<Sse>
std::unique_ptr<Sse> createSse() {
    return std::make_unique<Benchmarked<Sse>>();
}


template <class Dsse> requires IsDsse<Dsse>
std::unique_ptr<Dsse> createDsse(bool useShortcutSetup, bool shouldBenchmarkUpdts) {
    if (shouldBenchmarkUpdts) {
        return std::make_unique<BenchmarkedUpdts<Dsse>>(useShortcutSetup);
    } else {
        return std::make_unique<Benchmarked<Dsse>>(useShortcutSetup);
    }
}


} // namespace `app`
