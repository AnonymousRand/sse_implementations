#pragma once

#include <cstdint>
#include <random>

#include "utils/doc.h"
#include "utils/random.h"
#include "utils/range.h"
#include "utils/sse_utils.h"


namespace app {


Db<> createDb(int64_t dbSize, bool isRandom, bool hasDeletions) {
    if (dbSize == 0) {
        return Db<> {};
    }
    Db<> db;
    std::uniform_int_distribution<int64_t> dist(0, dbSize - 1);

    Id minId = 0;
    Id maxId = dbSize - 1;
    if (hasDeletions) {
        // delete the document with keyword 4
        Range<Kw> kwRangeDel {4, 4};
        db.push_back(DbEntry {Doc<> {0, 4, Op::INS, kwRangeDel}, kwRangeDel});
        db.push_back(DbEntry {Doc<> {0, 4, Op::DEL, kwRangeDel}, kwRangeDel});
        maxId -= 2;
    }

    if (isRandom) {
        // fill the rest with random keywords
        for (Id id = minId; id <= maxId; id++) {
            Kw kw = dist(utils::RNG);
            Range<Kw> kwRange {kw, kw};
            db.push_back(DbEntry {Doc<> {id, kw, Op::INS, kwRange}, kwRange});
        }
    } else {
        for (Id id = minId; id <= maxId; id++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // and make them non-contiguous to test Log-SRC as well
            Kw kw = (dbSize - id) * 2;
            Range<Kw> kwRange {kw, kw};
            db.push_back(DbEntry {Doc<> {id, kw, Op::INS, kwRange}, kwRange});
        }
    }

    return db;
}


} // namespace `app`
