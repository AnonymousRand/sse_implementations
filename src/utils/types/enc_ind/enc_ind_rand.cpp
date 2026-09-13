#include "utils/types/enc_ind/enc_ind_rand.h"

#include <cstdio>
#include <cstring>

#include "config.h"

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// `EncIndBase`


// TODO make this common in EncIndBase with pure virtual getters for bcktSize and bcktCount;
// then Loc sets them interactively as usual with the member variables while Rand forces them to 1
// and this->capacity respectively in the getters
bool EncIndRand::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one position at a time to search for it
    bigint positionsChecked = 0;
    uchar currEntry[this->ENTRY_LEN()];
    this->readEncoded(pos, currEntry, true);
    while (std::memcmp(currEntry, match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->capacity) {
            return false;
        }

        pos = (pos + 1) % this->capacity;
        if (pos == 0) {
            // the file pointer should be in the right position from the last `fread()` into
            // `buf` (and hence doesn't need an `fseek()`) IF AND ONLY IF we do not wrap around
            // (assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
            // (also, this can't be just an `fseek(0)` call here since we may only need to `fread()`
            // to fill `buf` again later on, when we need to read pointer to not still be at 0)
            this->readEncoded(pos, currEntry, true);
        } else {
            this->readEncoded(pos, currEntry, false);
        }
    }

    return true;
}
