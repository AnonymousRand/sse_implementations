#pragma once

#include <concepts>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>

#include "utils/disk_storage.h"
#include "utils/enc_ind.h"
#include "utils/tuple.h"


//==============================================================================
// `Db`
//==============================================================================


template <IsDbTuple DbTuple = Tuple<>>
class Db : public IDiskStorage {
public:
    inline static const int TUPLE_LEN = EncInd::DATA_LEN;

    Db();
    Db(const Db& db);

    ~Db();

    void clear();
    void append(const DbTuple& dbTuple);
    int64_t getSize() const;

    const DbTuple& operator [](int64_t index) const;

private:
    const std::string FILENAME_PREFIX() const override = {
        return "out/client/db_";
    }

    int64_t size = 0;
};


//==============================================================================
// `Ind`
//==============================================================================


template <IsDbTuple DbTuple>
using Ind = std::unordered_map<DbTuple, Db<DbTuple>>;
