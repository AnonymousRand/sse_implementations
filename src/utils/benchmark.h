#pragma once

#include <chrono>
#include <concepts>
#include <format>
#include <iostream>
#include <string>

#include "schemes/sse.h"
#include "utils/utils.h"


struct Benchmark {
    double time = 0;
    long communication = 0;

    void reset() {
        this->communication = 0;
    }

    static void printHeader() {
        std::cout << std::format("| {:<18} ", "Params")
                  << std::format("| {:<18} ", "Time (ms)")
                  << std::format("| {:<18} |", "Communication (B)")
                  << std::endl
                  << std::format("--------------------")
                  << std::format("---------------------")
                  << std::format("-----------------------")
                  << std::endl;
    }

    void print(const std::string& label) {
        std::cout << std::format("| {:<18} ", label)
                  << std::format("| {:<18} ", this->time)
                  << std::format("| {:<18} |", this->communication)
                  << std::endl;
    }

    void print(const std::string& label1, const std::string& label2) {
        std::cout << std::format("| {:<6} ", label1) << std::format("{:<11} ", label2)
                  << std::format("| {:<18} ", this->time)
                  << std::format("| {:<18} |", this->communication)
                  << std::endl;
    }
};


/**
 * Use this templated class as a "Python decorator" or "aspect" by using `Benchmarked<scheme>`
 * instead of just `scheme` when you wish to activate benchmarking features (along with having a
 * `Benchmark` member variable; unfortunately there doesn't seem to be an easy way to make one
 * automatically enforce the other).
 */
template <class Sse> requires IsSse<Sse>
class Benchmarked : public Sse {
    public:
        using Sse::Sse;

        void setup(int secParam, const Db<Doc<>, Kw>& db) override {
            this->benchmark.reset();

            auto start = std::chrono::high_resolution_clock::now();
            Sse::setup(secParam, db);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            this->benchmark.time = elapsed.count();
        }

        std::vector<Doc<>> search(
            const Range<Kw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const override {
            this->benchmark.reset();

            auto start = std::chrono::high_resolution_clock::now();
            std::vector<Doc<>> results = Sse::search(query, shouldCleanUpResults, isNaive);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            this->benchmark.time = elapsed.count();
            return results;
        }

        // (uncomment if you want to benchmark DSSE schemes per update instead of per setup)
        /*
        void update(const DbEntry<Doc<>, Kw>& newEntry) requires IsDsse<Sse> {
            this->benchmark.reset();

            auto start = std::chrono::high_resolution_clock::now();
            Sse::update(newEntry);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            this->benchmark.time = elapsed.count();
        }
        */
};
