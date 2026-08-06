#pragma once

#include "utils/utils.h"


struct Benchmark;


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
class ISseServer {
    public:
        // this should be the client/controller's benchmarking struct
        Benchmark& benchmark;

        ISseServer(Benchmark& benchmark) : benchmark(benchmark) {}

        virtual void clear();
};
