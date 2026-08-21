#include "utils/types/enc_ind/enc_ind_loc.h"

#include <cstdio>
#include <cstring>

#include "utils/benchmark.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/basic_types.h"


//------------------------------------------------------------------------------
// interface


void EncIndLoc::init(bigint bcktSize, bigint bcktCount) {
    EncIndBase::init(bcktSize * bcktCount);

    this->bcktSize = bcktSize;
    this->bcktCount = bcktCount;
}


void EncIndLoc::clear() {
    EncIndBase::clear();

    this->bcktSize = 0;
    this->bcktCount = 0;
}


//------------------------------------------------------------------------------
// helpers


bool EncIndLoc::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->size;

    // get entry at `pos`, and if it doesn't match `match` (e.g. because of `pos %= this->size`),
    // iterate forward by `this->bcktSize` positions at a time to search for it
    // 
    // importantly, we get the massive optimization of only having to check the first entry of every
    // bucket/every `this->bcktSize` entries, as locality guarantees contiguousness of buckets
    uchar currEntry[ENTRY_LEN];
    this->benchmark->startProfile("fseek");
    std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
    this->benchmark->stopProfile("fseek");
    this->readRaw(currEntry);
    bigint positionsChecked = 0;
    while (std::memcmp(currEntry, match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->bcktCount) {
            return false;
        }

        pos = (pos + this->bcktSize) % this->size;
        // if either we need to `fseek()` to further than we had `fread()` (i.e.
        // `this->bcktSize` > 1), or `pos` had been decreased this iteration (i.e.
        // `pos < this->bcktSize`) meaning we must've wrapped around, call `fseek()`
        // to make sure we are on the correct position (otherwise the previous `fread()`
        // automatically handles it, so we can save some time)
        if (this->bcktSize > 1 || pos < this->bcktSize) {
            this->benchmark->startProfile("fseek");
            std::fseek(this->file, pos * ENTRY_LEN, SEEK_SET);
            this->benchmark->stopProfile("fseek");
        }

        this->readRaw(currEntry);
    }
    
    return true;
}
