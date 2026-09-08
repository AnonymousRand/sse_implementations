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

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/range.h"
#include "types/tuple.h"


//==============================================================================
// `utils::benchmark`
//==============================================================================


namespace utils::benchmark {


// CONFIG
inline constexpr bigint PRINT_LABEL_WIDTH = 25;
inline constexpr bigint PRINT_LABEL_FIRST_HALF_WIDTH = 6;
inline constexpr bigint PRINT_COL_WIDTH = 18;


//--------------------------------------------------------------------------
// general stats to benchmark


inline double time = 0;
inline bigint serverStorage = 0;
inline bigint communication = 0;


//--------------------------------------------------------------------------
// averages (this should really only be done for ephemeral stats)


inline bigint totalUpdtCount = 0;
inline double totalUpdtTime = 0;
inline bigint totalUpdtCommunication = 0;

//--------------------------------------------------------------------------
// profiling


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
inline std::map<std::string, Profile> profiles;


inline void startProfile(const std::string& profileName) {
    auto iter = profiles.find(profileName);
    if (iter != profiles.end()) {
        iter->second.start = std::chrono::high_resolution_clock::now();
    } else {
        Profile profile;
        profile.start = std::chrono::high_resolution_clock::now();
        profiles.emplace(profileName, profile);
    }
}


inline void stopProfile(const std::string& profileName) {
    auto end = std::chrono::high_resolution_clock::now();

    auto iter = profiles.find(profileName);
    if (iter != profiles.end()) {
        Profile& profile = iter->second;
        std::chrono::duration<double, std::milli> elapsed = end - profile.start;
        profile.time += elapsed.count();
    } else {
        std::cerr << "Error: utils::benchmark::stopProfile(): attempted to end profile "
                  << profileName << ", but it has not been started yet" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


inline void resetProfile(const std::string& profileName) {
    auto iter = profiles.find(profileName);
    if (iter != profiles.end()) {
        iter->second.reset();
    }
}


//--------------------------------------------------------------------------
// utils


inline void resetAll() {
    time = 0;
    serverStorage = 0;
    communication = 0;

    totalUpdtCount = 0;
    totalUpdtTime = 0;
    totalUpdtCommunication = 0;

    profiles.clear();
}


inline void resetEphems() {
    time = 0;
    communication = 0;

    // reset profiles (but do not clear `profiles`, for speed and since it makes sense)
    for (auto& profilePair : profiles) {
        // (bypass the slower `resetProfile()` method here)
        profilePair.second.reset();
    }
}


//------------------------------------------------------------------------------
// printing


// note that we can't print profile headers here since they are not known beforehand,
// which is when this function is typically run, or in a inline context
// 
// the ugly workaround i have is just to print profile header names alongside their
// values inside the *body* of the table (in `print()` below)
inline void printHeader(bool shouldBenchmark) {
    if (shouldBenchmark) {
        std::cout << std::format("| {:<{}} ", "Params", PRINT_LABEL_WIDTH)
                  << std::format("| {:<{}} ", "Time (ms)", PRINT_COL_WIDTH)
                  << std::format("| {:<{}} ", "Server Storage (B)", PRINT_COL_WIDTH)
                  << std::format("| {:<{}} ", "Communication (B)", PRINT_COL_WIDTH)
                  // (trailing spaces to match bottom border, which should extend until
                  // the right border of the first profile output in the table body)
                  << std::format("| {:<{}}  ", "Profiling (ms) ...", PRINT_COL_WIDTH)
                  << std::endl;
        std::cout << std::format("--{:-<{}}-", "", PRINT_LABEL_WIDTH)
                  << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                  << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                  << std::format("--{:-<{}}-", "", PRINT_COL_WIDTH)
                  << std::format("--{:-<{}}--", "", PRINT_COL_WIDTH)
                  << std::endl;
    }
}


inline void print(bool shouldBenchmark, const std::string& label) {
    if (shouldBenchmark) {
        std::string profileOutputs = "";
        for (const auto& profilePair : profiles) {
            std::string profileName = profilePair.first;
            Profile profile = profilePair.second;
            // (you are advised to keep profile names short because of this :3)
            profileOutputs += std::format(
                "| {0:<{1}.{1}} ",
                std::format("{}: {}", profileName, profile.time), PRINT_COL_WIDTH
            );
        }


        std::cout << std::format("| {:<{}} ", label, PRINT_LABEL_WIDTH)
                  // explicitly cast doubles to string so that `.` controls exact string length,
                  // instead of sigfigs for doubles (where e.g. .01 & .10 are different lengths)
                  << std::format("| {0:<{1}.{1}} ", std::to_string(time), PRINT_COL_WIDTH)
                  << std::format("| {:<{}} ", serverStorage, PRINT_COL_WIDTH)
                  << std::format("| {:<{}} ", communication, PRINT_COL_WIDTH)
                  << profileOutputs << "|"
                  << std::endl;
    }
}


inline void print(bool shouldBenchmark, const std::string& label1, const std::string& label2) {
    std::string label = std::format(
        "{:<{}} {:<{}}",
        label1, PRINT_LABEL_FIRST_HALF_WIDTH,
        // (`- 1` because of the space between the first and second halves)
        label2, PRINT_LABEL_WIDTH - PRINT_LABEL_FIRST_HALF_WIDTH - 1
    );
    print(shouldBenchmark, label);
}


inline void printUpdtAvgs(bool shouldBenchmark, const std::string& label) {
    if (shouldBenchmark) {
        double avgUpdtTime          = totalUpdtTime          / totalUpdtCount;
        double avgUpdtCommunication = totalUpdtCommunication / totalUpdtCount;

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


} // namespace `utils::benchmark`


//==============================================================================
// `Benchmarked`
//==============================================================================


/**
 * use this templated class as a "Python decorator" or "aspect" by using `Benchmarked<scheme>`
 * instead of just `<scheme>` when you wish to activate benchmarking features.
 */
template <class Sse> requires IsSse<Sse>
class Benchmarked : public Sse {
public:
    using Sse::Sse;

    void setup(int secParam, const Db<Tuple<>>& db) override {
        utils::benchmark::resetAll();

        auto start = std::chrono::high_resolution_clock::now();
        Sse::setup(secParam, db);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        utils::benchmark::time = elapsed.count();
    }

    std::vector<Tuple<>> search(
        const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
    ) const override {
        utils::benchmark::resetEphems();

        auto start = std::chrono::high_resolution_clock::now();
        std::vector<Tuple<>> results = Sse::search(query, shouldCleanUpResults, isNaive);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        utils::benchmark::time = elapsed.count();

        return results;
    }
};


template <class Dsse> requires IsDsse<Dsse>
class BenchmarkedUpdts : public Benchmarked<Dsse> {
public:
    using Benchmarked<Dsse>::Benchmarked;

    // reset necessary benchmarks per update if using `BenchmarkedUpdts` (e.g. if
    // `config::SHOULD_BENCHMARK_UPDTS` is set to `true`)
    void update(const Tuple<>& newTuple) override {
        utils::benchmark::resetEphems();

        auto start = std::chrono::high_resolution_clock::now();
        Dsse::update(newTuple);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        utils::benchmark::time = elapsed.count();
        utils::benchmark::totalUpdtCount++;
        utils::benchmark::totalUpdtTime += utils::benchmark::time;
        utils::benchmark::totalUpdtCommunication += utils::benchmark::communication;
    }
};
