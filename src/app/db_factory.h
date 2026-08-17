#pragma once

#include <random>

#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app {


Db<> createDb(bigint dbSize, bool isRandom, bool hasDeletions) {
    Db<> db {};
    if (dbSize == 0) {
        // (note that we return the same `db` variable even with empty so that
        // compiler does named return value optimization)
        return db;
    }
    std::uniform_int_distribution<bigint> dist(0, dbSize - 1);

    Id minId = 0;
    Id maxId = dbSize - 1;
    if (hasDeletions) {
        // delete the document with keyword 4
        Range<Kw> kwRangeDel {4, 4};
        db.push_back(Tuple<> {0, 4, Op::INS, kwRangeDel});
        db.push_back(Tuple<> {0, 4, Op::DEL, kwRangeDel});
        maxId -= 2;
    }

    if (isRandom) {
        // fill the rest with random keywords
        for (Id id = minId; id <= maxId; id++) {
            Kw kw = dist(utils::random::RNG);
            Range<Kw> kwRange {kw, kw};
            db.push_back(Tuple<> {id, kw, Op::INS, kwRange});
        }
    } else {
        for (Id id = minId; id <= maxId; id++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // and make them non-contiguous to test Log-SRC as well
            Kw kw = (dbSize - id) * 2;
            Range<Kw> kwRange {kw, kw};
            db.push_back(Tuple<> {id, kw, Op::INS, kwRange});
        }
    }

    return db;
}


} // namespace `app`
