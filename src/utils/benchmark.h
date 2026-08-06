#pragma once


#include <iostream>

// TODO: make all imports prepend `schemes/` or `utils/`? or is that bad practice?
#include "schemes/sse.h"
#include "utils.h"


struct Benchmark {
    long totalComm = 0;

    void reset() {
        this->totalComm = 0;
    };

    friend std::ostream& operator <<(std::ostream& os, const Benchmark& benchmark) {
        os << "Benchmark: total communication " << benchmark.totalComm << " bytes";
    };
};


/**
 * Use this templated class as a "Python decorator" or "aspect" by using `Benchmarked<scheme>`
 * instead of just `scheme` when you wish to activate benchmarking features (along with having a
 * `Benchmark` member variable; unfortunately there doesn't seem to be an easy way to make one
 * automatically enforce the other).
 */
template <template <class ...> Sse> requires IsSse<Sse<Doc<>, Kw>>
class Benchmarked : public Sse {
    public:
        inline void setup(int secParam, const Db<DbDoc, DbKw>& db) = {
            this->benchmark.reset();
            Sse::setup(secParam, db);
        };

        inline std::vector<DbDoc> search(
            const Range<DbKw>& query, bool shouldCleanUpResults = true, bool isNaive = true
        ) const {
            this->benchmark.reset();
            Sse::search(query, shouldCleanUpResults, isNaive);
        };
};
