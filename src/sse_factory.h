#pragma once

#include <concepts>
#include <memory>

#include "benchmark.h"
#include "schemes/sse.h"


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
std::unique_ptr<Dsse> createDsse(bool shouldBenchmark, bool useShortcutSetup, bool shouldBenchmarkPerUpdt) {
    std::shared_ptr<Benchmark> benchmark = std::make_shared<Benchmark>();
    if (shouldBenchmark) {
        if (shouldBenchmarkPerUpdt) {
            return std::make_unique<BenchmarkedUpdt<Dsse>>(benchmark, useShortcutSetup);
        } else {
            return std::make_unique<Benchmarked<Dsse>>(benchmark, useShortcutSetup);
        }
    } else {
        return std::make_unique<Dsse>(benchmark, useShortcutSetup);
    }
}


template <class Sse> requires IsSse<Sse>
void deleteSse(Sse*& sse) {
    if (sse != nullptr) {
        delete sse;
        sse = nullptr;
    }
}
