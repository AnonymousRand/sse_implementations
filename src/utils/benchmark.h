#pragma once

#include <chrono>
#include <concepts>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "schemes/interfaces/dsse.h"
#include "schemes/interfaces/sse.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


struct Benchmark {
    double time = 0;
    bigint diskSize = 0;
    bigint network = 0;

    // (these are for tracking averages, and should really only be used for ephemeral stats)
    bigint totalUpdtCount = 0;
    double totalUpdtTime = 0;
    bigint totalUpdtNetwork = 0;

    void resetAll() {
        this->time = 0;
        this->diskSize = 0;
        this->network = 0;

        this->totalUpdtCount = 0;
        this->totalUpdtTime = 0;
        this->totalUpdtNetwork = 0;
    }

    void resetEphems() {
        this->time = 0;
        this->network = 0;
    }

    static void printHeader(bool shouldBenchmark) {
        if (shouldBenchmark) {
            std::cout << std::format("| {:<25} ", "Params")
                      << std::format("| {:<14} ", "Time (ms)")
                      << std::format("| {:<14} ", "Disk Size (B)")
                      << std::format("| {:<14} |", "Network (B)")
                      << std::endl
                      << std::format("----------------------------")
                      << std::format("-----------------")
                      << std::format("-----------------")
                      << std::format("------------------")
                      << std::endl;
        }
    }

    void print(bool shouldBenchmark, const std::string& label) const {
        if (shouldBenchmark) {
            std::cout << std::format("| {:<25} ", label)
                      // explicitly cast doubles to string so that `.` controls exact string length
                      << std::format("| {:<14.14} ", std::to_string(this->time))
                      << std::format("| {:<14} ", this->diskSize)
                      << std::format("| {:<14} |", this->network)
                      << std::endl;
        }
    }

    void print(bool shouldBenchmark, const std::string& label1, const std::string& label2) const {
        std::string label = std::format("{:<6} {:<18}", label1, label2);
        this->print(shouldBenchmark, label);
    }

    void printUpdtAvgs(bool shouldBenchmark, const std::string& label) const {
        if (shouldBenchmark) {
            double avgUpdtTime    = this->totalUpdtTime    / this->totalUpdtCount;
            double avgUpdtNetwork = this->totalUpdtNetwork / this->totalUpdtCount;

            std::cout << std::format("| {:<25} ", label)
                      << std::format("| {:<14.14} ", std::to_string(avgUpdtTime))
                      << std::format("| {:<14} ", "")
                      << std::format("| {:<14.14} |", std::to_string(avgUpdtNetwork))
                      << std::endl;
        }
    }
};


/**
 * use this templated class as a "Python decorator" or "aspect" by using `Benchmarked<scheme>`
 * instead of just `scheme` when you wish to activate benchmarking features (along with having a
 * `Benchmark` member variable; unfortunately there doesn't seem to be an easy way to make one
 * automatically enforce the other).
 */
template <class Sse> requires IsSse<Sse>
class Benchmarked : public Sse {
public:
    using Sse::Sse;

    void setup(int secParam, const Db<Tuple<>>& db) override {
        this->benchmark->resetAll();

        auto start = std::chrono::high_resolution_clock::now();
        Sse::setup(secParam, db);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        this->benchmark->time = elapsed.count();
    }

    std::vector<Tuple<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override {
        this->benchmark->resetEphems();

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Tuple<>> results = Sse::search(query, shouldCleanUpResults, isNaive);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        this->benchmark->time = elapsed.count();

        return results;
    }
};


template <class Dsse> requires IsDsse<Dsse>
class BenchmarkedUpdts : public Benchmarked<Dsse> {
public:
    using Benchmarked<Dsse>::Benchmarked;

    // reset necessary benchmarks per update if using `BenchmarkedUpdts` (e.g. if
    // `config::DSSE_SHOULD_BENCHMARK_UPDTS` is set to `true`)
    void update(const Tuple<>& newTuple) override {
        this->benchmark->resetEphems();

        auto start = std::chrono::high_resolution_clock::now();
        Dsse::update(newTuple);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        this->benchmark->time = elapsed.count();
        this->benchmark->totalUpdtCount++;
        this->benchmark->totalUpdtTime += this->benchmark->time;
        this->benchmark->totalUpdtNetwork += this->benchmark->network;
    }
};
