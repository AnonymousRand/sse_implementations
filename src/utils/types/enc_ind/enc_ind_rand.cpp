#include "utils/types/enc_ind/enc_ind_rand.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "config.h"

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"
#include "utils/types/ustring.h"


//==============================================================================
// `EncIndRand`
//==============================================================================


//------------------------------------------------------------------------------
// `EncIndBase`


bool EncIndRand::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one position at a time to search for it
    // additionally, we use a buffer in memory to speed up long chains of iterating forward
    // one position at a time (at the cost of some worse performance at smaller sizes)
    const ubigint origStartPos = pos;
    const bigint readBufEntryCapacity = std::min(config::ENC_IND_READ_BUF_CAPACITY, this->capacity);
    uchar readBuf[readBufEntryCapacity * this->ENTRY_LEN()];
    bigint readBufEntryCount = this->readIntoReadBuf(
        readBuf, readBufEntryCapacity, pos, origStartPos, true
    );
    bigint readBufIndex = 0;
    bool needsFseek = false;
    bigint positionsChecked = 0;
    while (std::memcmp(readBuf + (readBufIndex * this->ENTRY_LEN()), match, matchLen) != 0) {
        positionsChecked++;
        if (positionsChecked == this->capacity) {
            return false;
        }

        pos = (pos + 1) % this->capacity;
        if (pos == 0) {
            // the file pointer should be in the right position from the last `fread()` into
            // `readBuf` (and hence doesn't need an `fseek()`) IF AND ONLY IF we do not wrap around
            // (assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
            // (also, this can't be just an `fseek(0)` call here since we may only need to `fread()`
            // to fill `readBuf` again later on, when we need to read pointer to not still be at 0)
            needsFseek = true;
        }

        // (this must come before we set `readBufIndex` to 0, or else we skip over an entry)
        readBufIndex++;
        // if we've read to the end of the current `readBuf`, read the next part of the file into it
        if (readBufIndex >= readBufEntryCount) {
            readBufEntryCount = this->readIntoReadBuf(
                readBuf, readBufEntryCapacity, pos, origStartPos, needsFseek
            );
            readBufIndex = 0;
            needsFseek = false;
        }
    }

    return true;
}


//------------------------------------------------------------------------------
// helpers


bigint EncIndRand::readIntoReadBuf(
    uchar* readBuf, bigint targetEntryCount, ubigint readBufStartPos, ubigint origStartPos,
    bool needsFseek
) const {
    bigint entriesUntilEof = this->capacity - readBufStartPos;
    bigint entriesUntilFullLoop;
    if (readBufStartPos < origStartPos)      entriesUntilFullLoop = origStartPos - readBufStartPos;
    else if (readBufStartPos > origStartPos) entriesUntilFullLoop = entriesUntilEof + origStartPos;
    else                                     entriesUntilFullLoop = this->capacity;
    // we want to make sure we don't exceed where we had started doing this whole thing back in
    // the caller (e.g. if we had already wrapped around and are getting close to a full loop)
    bigint entriesToRead = std::min(targetEntryCount, entriesUntilFullLoop);

    bigint entriesToReadUntilEof = std::min(entriesToRead, entriesUntilEof);
    utils::benchmark::startProfile("fseek");
    if (needsFseek) {
        std::fseek(this->file, readBufStartPos * this->ENTRY_LEN(), SEEK_SET);
    }
    utils::benchmark::stopProfile("fseek");
    utils::benchmark::startProfile("fread");
    bigint itemsRead = std::fread(readBuf, this->ENTRY_LEN(), entriesToReadUntilEof, this->file);
    utils::benchmark::stopProfile("fread");
    DEBUG_ONLY({
        if (itemsRead < entriesToReadUntilEof) {
            std::cerr << "Error: EncIndRand::readIntoReadBuf(): error reading (part 1) "
                      << "from file " << this->filename
                      << " (only read " << itemsRead << " out of " << entriesToReadUntilEof << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // wrap around to beginning of file if we read less than the target number of entries
    if (entriesToReadUntilEof < entriesToRead) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, 0, SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        utils::benchmark::startProfile("fread");
        itemsRead += std::fread(
            readBuf + (entriesToReadUntilEof * this->ENTRY_LEN()),
            this->ENTRY_LEN(), entriesToRead - entriesToReadUntilEof,
            this->file
        );
        utils::benchmark::stopProfile("fread");
        DEBUG_ONLY({
            if (itemsRead < entriesToRead) {
                std::cerr << "Error: EncIndRand::writeToFirstEmpty(): error reading (part 2) "
                          << "from file " << this->filename
                          << " (only read " << itemsRead << " out of " << entriesToRead << ")"
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
    
    return itemsRead;
}
