#pragma once

#include <concepts>
#include <memory>

#include "schemes/interfaces/sse.h"

#include "utils/benchmark.h"


namespace app {


template <class Sse> requires IsSse<Sse>
std::unique_ptr<Sse> createSse(bool shouldBenchmark) {
    std::shared_ptr<Benchmark> benchmark = std::make_shared<Benchmark>();
    if (shouldBenchmark) {
        return std::make_unique<Benchmarked<Sse>>(benchmark);
    } else {
        return std::make_unique<Sse>(benchmark);
    }
}


template <class Dsse> requires IsDsse<Dsse>
std::unique_ptr<Dsse> createDsse(bool shouldBenchmark, bool useShortcutSetup, bool shouldBenchmarkUpdts) {
    std::shared_ptr<Benchmark> benchmark = std::make_shared<Benchmark>();
    if (shouldBenchmark) {
        if (shouldBenchmarkUpdts) {
            return std::make_unique<BenchmarkedUpdts<Dsse>>(benchmark, useShortcutSetup);
        } else {
            return std::make_unique<Benchmarked<Dsse>>(benchmark, useShortcutSetup);
        }
    } else {
        return std::make_unique<Dsse>(benchmark, useShortcutSetup);
    }
}


} // namespace `app`
