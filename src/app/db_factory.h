#pragma once

#include <random>

#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


namespace app {


/**
 * add `dbSize` new tuples to the DB `ret`. if `isRandom` is `true`, then the keywords
 * are limited to between `minKw` and `maxKw` (both inclusive) (do note that `hasDeletions`
 * should be set to `false` in this case).
 */
void createDb(
    Db<>& ret, bigint dbSize, bool isRandom, bool hasDeletions,
    bigint minKw = DUMMY, bigint maxKw = DUMMY
) {
    if (dbSize == 0) {
        return;
    }

    // default param values (they can't depend on earlier function params)
    if (minKw == DUMMY) { minKw = 0; }
    if (maxKw == DUMMY) { maxKw = dbSize + minKw - 1; }

    Id minId = minKw;
    Id maxId = dbSize + minKw - 1;
    if (hasDeletions) {
        // delete the document with keyword 4
        Range<Kw> kwRangeDel {4, 4};
        ret.push_back(Tuple<> {0, 4, Op::INS, kwRangeDel});
        ret.push_back(Tuple<> {0, 4, Op::DEL, kwRangeDel});
        maxId -= 2;
    }

    if (isRandom) {
        // fill the rest with random keywords
        std::uniform_int_distribution<bigint> dist(minKw, maxKw);
        for (Id id = minId; id <= maxId; id++) {
            Kw kw = dist(utils::random::RNG);
            Range<Kw> kwRange {kw, kw};
            ret.push_back(Tuple<> {id, kw, Op::INS, kwRange});
        }
    } else {
        for (Id id = minId; id <= maxId; id++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // and make them non-contiguous to test Log-SRC as well
            Kw kw = (dbSize - id) * 2;
            Range<Kw> kwRange {kw, kw};
            ret.push_back(Tuple<> {id, kw, Op::INS, kwRange});
        }
    }
}


} // namespace `app`
