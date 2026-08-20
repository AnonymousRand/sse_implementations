#pragma once

#include <chrono>
#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "schemes/interfaces/dsse.h"
#include "schemes/interfaces/sse.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


struct Benchmark {
private:
    // config
    inline static constexpr bigint PRINT_LABEL_WIDTH = 25;
    inline static constexpr bigint PRINT_LABEL_FIRST_HALF_WIDTH = 6;
    inline static constexpr bigint PRINT_COL_WIDTH = 18;

public:
    //--------------------------------------------------------------------------
    // general stats to benchmark

    double time = 0;
    bigint serverStorage = 0;
    bigint communication = 0;

    //--------------------------------------------------------------------------
    // tracking averages (this should really only be done for ephemeral stats)

    bigint totalUpdtCount = 0;
    double totalUpdtTime = 0;
    bigint totalUpdtCommunication = 0;

    //--------------------------------------------------------------------------
    // profiling specific pieces of code

    // IMPORTANT: this does not currently work with nested profilings on the same `benchmark`
    // object and profile!
    struct Profile {
        double time = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> start;

        void reset() {
            this->time = 0;
        }
    };

    // (this is a `map` instead of an `unordered_map` since it's probably slightly faster
    // at small scales like these, and plus the ordering is a nice bonus)
    std::map<std::string, Profile> profiles;

    void startProfile(const std::string& profileName) {
        auto iter = this->profiles.find(profileName);
        if (iter != this->profiles.end()) {
            iter->second.start = std::chrono::high_resolution_clock::now();
        } else {
            Profile profile;
            profile.start = std::chrono::high_resolution_clock::now();
            this->profiles.emplace(profileName, profile);
        }
    }

    void endProfile(const std::string& profileName) {
        auto end = std::chrono::high_resolution_clock::now();

        auto iter = this->profiles.find(profileName);
        if (iter != this->profiles.end()) {
            Profile& profile = iter->second;
            std::chrono::duration<double, std::milli> elapsed = end - profile.start;
            profile.time += elapsed.count();
        } else {
            std::cerr << "Error: Benchmark::endProfile(): attempted to end profile "
                      << profileName << ", but it has not been started yet" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    void resetProfile(const std::string& profileName) {
        auto iter = this->profiles.find(profileName);
        if (iter != this->profiles.end()) {
            iter->second.reset();
        }
    }

    //--------------------------------------------------------------------------
    // utils

    void resetAll() {
        this->time = 0;
        this->serverStorage = 0;
        this->communication = 0;

        this->totalUpdtCount = 0;
        this->totalUpdtTime = 0;
        this->totalUpdtCommunication = 0;

        this->profiles.clear();
    }

    void resetEphems() {
        this->time = 0;
        this->communication = 0;

        // reset profiles (but do not clear `this->profiles`, for speed and since it makes sense)
        for (auto& profilePair : this->profiles) {
            // bypass slower `resetProfile()` method here
            profilePair.second.reset();
        }
    }

    // note that we can't print profile headers here since they are not known beforehand,
    // which is when this function is typically run, or in a static context
    // 
    // the ugly workaround i have is just to print profile header names alongside their
    // values inside the *body* of the table (in `print()` below)
    static void printHeader(bool shouldBenchmark) {
        if (shouldBenchmark) {
            std::cout << std::format("| {:<{}} ", "Params", PRINT_LABEL_WIDTH)
                      << std::format("| {:<{}} ", "Time (ms)", PRINT_COL_WIDTH)
                      << std::format("| {:<{}} ", "Server Storage (B)", PRINT_COL_WIDTH)
                      << std::format("| {:<{}} ", "Communication (B)", PRINT_COL_WIDTH)
                      // (trailing spaces to match bottom border, which should extend until
                      // the right border of the first profile output in the table body)
                      << std::format("| {:<{}}  ", "Profiling (ms)", PRINT_COL_WIDTH)
                      << std::endl;
            std::cout << std::format("--{:-<{}}-", "", PRINT_LABEL_WIDTH)
                      << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                      << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                      << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                      << std::format("--{:-<{}}--", "", PRINT_COL_WIDTH)
                      << std::endl;
        }
    }

    void print(bool shouldBenchmark, const std::string& label) const {
        if (shouldBenchmark) {
            std::string profileOutputs = "";
            for (const auto& profilePair : this->profiles) {
                std::string profileName = profilePair.first;
                Profile profile = profilePair.second;
                // (you are advised to keep profile names short because of this :3)
                profileOutputs += std::format(
                    "| {0:<{1}.{1}} ", std::format(
                        "{}: {}", profileName, profile.time
                    ), PRINT_COL_WIDTH
                );
            }


            std::cout << std::format("| {:<{}} ", label, PRINT_LABEL_WIDTH)
                      // explicitly cast doubles to string so that `.` controls exact string length
                      << std::format("| {0:<{1}.{1}} ", std::to_string(this->time), PRINT_COL_WIDTH)
                      << std::format("| {:<{}} ", this->serverStorage, PRINT_COL_WIDTH)
                      << std::format("| {:<{}} ", this->communication, PRINT_COL_WIDTH)
                      << profileOutputs << "|"
                      << std::endl;
        }
    }

    void print(bool shouldBenchmark, const std::string& label1, const std::string& label2) const {
        std::string label = std::format(
            "{:<{}} {:<{}}",
            label1, PRINT_LABEL_FIRST_HALF_WIDTH,
            // (`- 1` because of the space between the first and second halves)
            label2, PRINT_LABEL_WIDTH - PRINT_LABEL_FIRST_HALF_WIDTH - 1
        );
        this->print(shouldBenchmark, label);
    }

    void printUpdtAvgs(bool shouldBenchmark, const std::string& label) const {
        if (shouldBenchmark) {
            double avgUpdtTime    = this->totalUpdtTime    / this->totalUpdtCount;
            double avgUpdtCommunication = this->totalUpdtCommunication / this->totalUpdtCount;

            std::cout << std::format("| {:<25} ", label)
                      << std::format(
                          "| {0:<{1}.{1}} ", std::to_string(avgUpdtTime), PRINT_COL_WIDTH
                      )
                      << std::format("| {:<{}} ", "-", PRINT_COL_WIDTH)
                      << std::format(
                          "| {0:<{1}.{1}} |", std::to_string(avgUpdtCommunication), PRINT_COL_WIDTH
                      )
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
        this->benchmark->totalUpdtCommunication += this->benchmark->communication;
    }
};
