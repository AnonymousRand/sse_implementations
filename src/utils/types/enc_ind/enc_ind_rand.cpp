#include "utils/types/enc_ind/enc_ind_rand.h"

#include <algorithm>
#include <cassert>
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
// `EncIndRand::Buf`
//==============================================================================


const bigint EncIndRand::Buf::INVALID_INDEX = -1;


EncIndRand::Buf::Buf(bigint ENTRY_CAPACITY, bigint entryLen) : ENTRY_CAPACITY(ENTRY_CAPACITY) {
    this->data = new uchar[this->ENTRY_CAPACITY * entryLen];
}


EncIndRand::Buf::~Buf() {
    if (this->data != nullptr) {
        delete[] this->data;
        this->data = nullptr;
    }
}


//==============================================================================
// `EncIndRand`
//==============================================================================


//------------------------------------------------------------------------------
// interface


void EncIndRand::init(bigint capacity) {
    EncIndBase::init(capacity);

    bigint bufEntryCapacity = std::min(config::ENC_IND_READ_BUF_CAPACITY, this->capacity);
    this->buf = new Buf(bufEntryCapacity, this->ENTRY_LEN());
}


void EncIndRand::clear() {
    if (this->buf != nullptr) {
        delete this->buf;
        this->buf = nullptr;
    }

    EncIndBase::clear();
}


//------------------------------------------------------------------------------
// `EncIndBase`


bool EncIndRand::advanceUntilMatch(ubigint& pos, const uchar* match, int matchLen) const {
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward one position at a time to search for it
    //std::cout << "----- getting read buf index for pos " << pos << " with curr read buf " << this->currBufStartPos << ", " << this->currBufEndPos << "; cap " << this->capacity << std::endl;
    const ubigint origStartPos = pos;
    bigint bufIndex = this->posToBufIndex(pos);
    if (bufIndex == Buf::INVALID_INDEX) {
        this->readIntoBuf(pos, origStartPos, true);
        //std::cout << "+++ not in current read buf!! new read buf obtained: " << this->currBufStartPos << ", " << this->currBufEndPos << "; cap " << this->capacity << std::endl;
        bufIndex = 0;
    }
    //std::cout << "computed read buf index for pos " << pos << " is " << bufIndex << std::endl;
    assert(bufIndex < this->currBufEntryCount);
    bool needsFseek = false;
    bigint positionsChecked = 0;
    while (std::memcmp(this->buf + (bufIndex * this->ENTRY_LEN()), match, matchLen) != 0) {
        //std::cout << "checked: read buf index is " << bufIndex << " and pos is " << pos << std::endl;
        //std::cout << "current spot had " << utils::debug::ustrToHex(this->buf + (bufIndex * this->ENTRY_LEN()), 16) << std::endl;
        positionsChecked++;
        if (positionsChecked == this->capacity) {
            std::cout << "failed!" << std::endl;
            return false;
        }

        pos = (pos + 1) % this->capacity;
        if (pos == 0) {
            // the file pointer should be in the right position from the last `fread()` into
            // `buf` (and hence doesn't need an `fseek()`) IF AND ONLY IF we do not wrap around
            // (assuming no other `fread()`s, `fwrite()`s, or `fseek()`s have occurred since then)
            // (also, this can't be just an `fseek(0)` call here since we may only need to `fread()`
            // to fill `buf` again later on, when we need to read pointer to not still be at 0)
            needsFseek = true;
        }

        // (this can't come after we set `bufIndex` to 0, or else we skip over an entry)
        bufIndex = this->posToBufIndex(pos);
        if (bufIndex == Buf::INVALID_INDEX) {
        // if we've read to the end of the current `buf`, read the next part of the file into it
            this->readIntoBuf(pos, origStartPos, needsFseek);
            //std::cout << "+++ regenerating new read buf: " << this->currBufStartPos << ", " << this->currBufEndPos << "; cap " << this->capacity << std::endl;
            bufIndex = 0;
            needsFseek = false;
        }
    }

    return true;
}


// so: on writeToFirstEmpty to `pos`: first compute posToBufIndex(pos). if negative, flush current
// buf to disk if it's not empty!! and then read new buf starting at `pos`. then find a spot as usual
// with advanceUntilMatch. then finally for write, we write into the buffer too after computing
// bufIndex again for the final `pos`

// on find at `pos`: same thing? first compute posToBufIndex(pos). if negative, flush current buf
// to disk if it's not empty, and then read new buf starting at `pos`. then find the spot as usual
// with advanceUntilMatch. then finally for read, read from the buffer too after computing bufIndex
// again for the final `pos`.

// maybe faster? way: if negative, only read one entry into buf first, and only if that doesn't match,
// then read remaning length of buf?

// hmm but this still feels like there isn't much opportunity for cases where the buffer gets too
// remain unchanged between two different writes/finds, which is the only timesave this optimization
// has, due to pseudorandom assignments. the buffer really should only help with the scanning for
// available spot inside a single write/find? test % of times we get to use this optimization in
// a separate branch with original impl.

// so: override not just advanceUntilMatch but both readEncoded and writeEncoded too to read/write
// with buf instead of file. advanceUntilMatch should be basically same as here except with the flush
// to disk process? one thing though: it feels weird to put the flushing into advanceUntilMatch no?
void EncIndRand::writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek) {
    bigint bufIndex = this->posToBufIndex(pos);
    if (bufIndex == Buf::INVALID_INDEX) {
        if (this->buf->entryCount > 0) {
            this->flushBufToFile();
        }

        this->readIntoBuf(pos, pos, shouldFseek);
        bufIndex = 0;
    }

    


    if (shouldFseek) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
    }
    utils::benchmark::startProfile("fwrite");
    int itemsWritten = std::fwrite(encodedEntry, this->ENTRY_LEN(), 1, this->file);
    utils::benchmark::stopProfile("fwrite");
    DEBUG_ONLY({
        if (itemsWritten != 1) {
            std::cerr << "Error: EncIndBase::writeEncoded(): error writing to file "
                      << this->filename << " (nothing written)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
    this->isFlushed = false;
}


//------------------------------------------------------------------------------
// helpers


void EncIndRand::readIntoBuf(ubigint bufStartPos, ubigint origStartPos, bool needsFseek) const {
    bigint entriesUntilEof = this->capacity - bufStartPos;
    bigint entriesUntilFullLoop;
    if (bufStartPos < origStartPos)      entriesUntilFullLoop = origStartPos - bufStartPos;
    else if (bufStartPos > origStartPos) entriesUntilFullLoop = entriesUntilEof + origStartPos;
    else                                     entriesUntilFullLoop = this->capacity;
    // we want to make sure we don't exceed where we had started doing this whole thing back in
    // the caller (e.g. if we had already wrapped around and are getting close to a full loop)
    bigint entriesToRead = std::min(this->bufEntryCapacity, entriesUntilFullLoop);

    bigint entriesToReadUntilEof = std::min(entriesToRead, entriesUntilEof);
    utils::benchmark::startProfile("fseek");
    if (needsFseek) {
        std::fseek(this->file, bufStartPos * this->ENTRY_LEN(), SEEK_SET);
    }
    utils::benchmark::stopProfile("fseek");
    utils::benchmark::startProfile("fread");
    bigint itemsRead = std::fread(
        this->buf, this->ENTRY_LEN(), entriesToReadUntilEof, this->file
    );
    utils::benchmark::stopProfile("fread");
    DEBUG_ONLY({
        if (itemsRead < entriesToReadUntilEof) {
            std::cerr << "Error: EncIndRand::readIntoBuf(): error reading (part 1) "
                      << "from file " << this->filename
                      << " (only read " << itemsRead << " out of " << entriesToReadUntilEof << ")"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // wrap around to beginning of file if we read less than the target number of entries
    // (NOTE: the read buf must not be larger than `this->capacity`, so that we
    // only need to wrap around at most once!)
    if (entriesToReadUntilEof < entriesToRead) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, 0, SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        utils::benchmark::startProfile("fread");
        itemsRead += std::fread(
            this->buf + (entriesToReadUntilEof * this->ENTRY_LEN()),
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
    
    this->currBufStartPos = bufStartPos;
    this->currBufEndPos = (bufStartPos + itemsRead) % this->capacity;
    this->currBufEntryCount = itemsRead;
    std::cout << "current end pos is now " << this->currBufEndPos << " while actual current end pos is " << (float)std::ftell(this->file) / this->ENTRY_LEN() << std::endl;
}


bigint EncIndRand::posToBufIndex(ubigint pos) const {
    if (this->buf->entryCount == 0) {
        return Buf::INVALID_INDEX;
    }

    if (this->buf->startPos < this->buf->endPos) {
        if (pos >= this->buf->startPos && pos < this->buf->endPos) {
            // if `this->buf` did not reach or wrap around the end of the file
            // and `pos` is in the middle of it (note that this also means the following
            // returned value should always be positive)
            bigint ret = pos - this->buf->startPos;
            assert(ret >= 0);
            return ret;
        }
    } else {
        // if `this->buf` does reach or wrap around the end of the file
        if (pos >= this->buf->startPos) {
            // if `pos` is at/after the buffer's start pos (so `pos` hasn't wrapped around yet)
            bigint ret = pos - this->buf->startPos;
            assert(ret >= 0);
            return ret;
        } else if (pos < this->buf->endPos) {
            // if `pos` is before the buffer's end pos (so `pos` did wrap around)

            // we add up the segment from `pos` to the start of the enc ind,
            // and the segment from the end of the enc ind to `this->startPos`
            bigint ret = pos + (this->capacity - this->buf->startPos);
            assert(ret >= 0);
            return ret;
        }
    }
    return Buf::INVALID_INDEX;
}
