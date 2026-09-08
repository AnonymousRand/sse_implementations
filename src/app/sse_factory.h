#pragma once

#include <concepts>
#include <memory>

#include "schemes/interfaces/dsse.h"
#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"


namespace app {


template <class Sse> requires IsSse<Sse>
std::unique_ptr<Sse> createSse(bool shouldBenchmark) {
    if (shouldBenchmark) {
        return std::make_unique<Benchmarked<Sse>>();
    } else {
        return std::make_unique<Sse>();
    }
}


template <class Dsse> requires IsDsse<Dsse>
std::unique_ptr<Dsse> createDsse(
    bool shouldBenchmark, bool useShortcutSetup, bool shouldBenchmarkUpdts
) {
    if (shouldBenchmark) {
        if (shouldBenchmarkUpdts) {
            return std::make_unique<BenchmarkedUpdts<Dsse>>(useShortcutSetup);
        } else {
            return std::make_unique<Benchmarked<Dsse>>(useShortcutSetup);
        }
    } else {
        return std::make_unique<Dsse>(useShortcutSetup);
    }
}


} // namespace `app`
