#include "utils/types/enc_ind/enc_ind_base.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "config.h"

#include "utils/benchmark.h"
#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/i_disk_storage.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// constructors/destructors


EncIndBase::~EncIndBase() {
    this->clear();
}


//------------------------------------------------------------------------------
// rule of five


// copy constructor
EncIndBase::EncIndBase(const EncIndBase& other) {
    IDiskStorage::copyFrom(other);
}


//------------------------------------------------------------------------------
// interface


void EncIndBase::init(bigint capacity) {
    // inits DB file and file pointer
    IDiskStorage::init();

    // this initializes `this->NULL_ENTRY` to a contiguous block of zero bits, which we do here
    // instead of in the constructor since `this->ENTRY_LEN()()` relies on virtual methods
    // (technically it is possible that an encrypted tuple happens to be all '0' bytes and thus gets
    // mistaken for a null kv pair, but currently `this->ENTRY_LEN()` is >1000 bits so there's
    // a 2^{>1000} chance of this happening...and USENIX'24's implementation just does this too)
    this->NULL_ENTRY = new uchar[this->ENTRY_LEN()] {};
    this->capacity = capacity;

    // fill file with zero bits
    utils::benchmark::startProfile("init");
    for (bigint i = 0; i < this->capacity; i++) {
        int itemsWritten = std::fwrite(this->NULL_ENTRY, this->ENTRY_LEN(), 1, this->file);
        DEBUG_ONLY({
            if (itemsWritten != 1) {
                std::cerr << "Error: EncIndBase::init(): error initializing file " << this->filename
                          << " with zero bits (nothing written)" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
    std::fflush(this->file);
    utils::benchmark::stopProfile("init");
}


void EncIndBase::clear() {
    if (this->NULL_ENTRY != nullptr) {
        delete[] this->NULL_ENTRY;
        this->NULL_ENTRY = nullptr;
    }
    this->capacity = 0;

    // clears DB file and file pointer
    IDiskStorage::clear();
}


bool EncIndBase::read(ubigint pos, EncIndVal& ret, bool shouldFseek) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    if (shouldFseek) {
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
    }
    this->readEncoded(entry);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndVal::fromUcstr(entry + this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
}


bool EncIndBase::find(ubigint& pos, const ustring& key, EncIndVal& ret) const {
    bool isFound = this->advanceUntilMatch(
        pos, key.c_str(), this->KEY_LEN(), config::SHOULD_BUFFER_SEARCH
    );
    if (!isFound) {
        return false;
    }

    // read and decode the kv pair at the matched location we found
    return this->read(pos, ret);
}


void EncIndBase::write(ubigint pos, const EncIndEntry& encIndEntry, bool shouldFseek) {
    pos %= this->capacity;

    // encode `encIndEntry` into one string
    ustring encodedEntry = encIndEntry.toUstr();
    DEBUG_ONLY({
        if (encodedEntry.length() != this->ENTRY_LEN()) {
            std::cerr << "Error: EncIndBase::write(): write of length " << encodedEntry.length()
                      << " bytes is not allowed! (want " << this->ENTRY_LEN() << " bytes)"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // then go to `pos` and write the encoded `encIndEntry`
    this->writeEncoded(pos, encodedEntry.c_str(), shouldFseek);
}


void EncIndBase::writeToFirstEmpty(ubigint& pos, const EncIndEntry& encIndEntry) {
    bool isEmptyAvailable = this->advanceUntilMatch(
        pos, this->NULL_ENTRY, this->ENTRY_LEN(), config::SHOULD_BUFFER_SETUP
    );
    // if we've scoured the whole index and still haven't found an available space,
    // throw an error: we are trying to write to a full index
    DEBUG_ONLY({
        if (!isEmptyAvailable) {
            std::cerr << "Error: EncIndBase::writeToFirstEmpty(): ran out of space writing to "
                      << this->filename << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });

    // write into the empty location we found
    this->write(pos, encIndEntry);
}


void EncIndBase::print() const {
    for (bigint pos = 0; pos < this->capacity; pos++) {
        EncIndEntry encIndEntry;
        this->readEntry(pos, encIndEntry);
        std::cerr << pos << ": " << utils::debug::ustrToHex(encIndEntry.toUstr())
                  << std::endl << std::endl;
    }
}


//------------------------------------------------------------------------------
// helpers


void EncIndBase::readEncoded(uchar* buf) const {
    utils::benchmark::startProfile("fflush");
    this->flushIfNotFlushed();
    utils::benchmark::stopProfile("fflush");

    utils::benchmark::startProfile("fread");
    bigint itemsRead = std::fread(buf, this->ENTRY_LEN(), 1, this->file);
    utils::benchmark::stopProfile("fread");
    DEBUG_ONLY({
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndBase::readEncoded(): error reading from file "
                      << this->filename << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
}


void EncIndBase::writeEncoded(ubigint pos, const uchar* encodedEntry, bool shouldFseek) {
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


bool EncIndBase::advanceUntilMatch(
    ubigint& pos, const uchar* match, int matchLen, bool shouldBuffer
) const {
    pos %= this->capacity;

    // get entry at `pos`, and if it doesn't match `match` (e.g. due to `pos %= this->capacity`),
    // iterate forward `this->getBcktSize()` positions at a time to search for it
    uchar firstEntry[this->ENTRY_LEN()];
    std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    int itemsRead = std::fread(firstEntry, this->ENTRY_LEN(), 1, this->file);
    DEBUG_ONLY({
        if (itemsRead != 1) {
            std::cerr << "Error: EncIndRand::advanceUntilMatch(): error reading from file "
                      << this->filename << " (nothing read)" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    });
    if (std::memcmp(firstEntry, match, matchLen) == 0) {
        return true;
    }

    if (shouldBuffer && this->getBcktSize() <= 2) {
        // if we do need to iterate forward, we can use a buffer in memory to speed up long chains
        // of iterating forward a small number of (i.e. `this->getBcktSize()`) positions at a time
        const ubigint origStartPos = pos;
        const bigint readBufEntryCapacity = std::min(
            config::ENC_IND_READ_BUF_CAPACITY, this->capacity
        );
        uchar readBuf[readBufEntryCapacity * this->ENTRY_LEN()];
        bigint readBufEntryCount = this->readIntoReadBuf(
            // technically we are repeating the first read again, but doing `+ 1` breaks so whatever
            readBuf, readBufEntryCapacity, pos, origStartPos, true
        );
        bigint readBufIndex = 0;
        bool needsFseek = false;
        bigint positionsChecked = 0;
        while (std::memcmp(readBuf + (readBufIndex * this->ENTRY_LEN()), match, matchLen) != 0) {
            positionsChecked++;
            if (positionsChecked == this->getBcktCount()) {
                return false;
            }

            pos = (pos + this->getBcktSize()) % this->capacity;
            if (this->getBcktSize() > 1 || pos < this->getBcktSize()) {
                // if either we need to `fseek()` to further than we had `fread()` (i.e.
                // `this->getBcktSize()` > 1), or `pos` had been decreased this iteration (i.e.
                // `pos < this->getBcktSize()`) meaning we must've wrapped around, call `fseek()`
                // to make sure we are on the correct position (otherwise the previous `fread()`
                // automatically handles it, so we can save some time)
                needsFseek = true;
            }

            // (this must come before we set `readBufIndex` to 0, or else we skip over an entry)
            readBufIndex += this->getBcktSize();
            // if we've read to the end of `readBuf`, read the next part of the file into it
            if (readBufIndex >= readBufEntryCount) {
                readBufEntryCount = this->readIntoReadBuf(
                    readBuf, readBufEntryCapacity, pos, origStartPos, needsFseek
                );
                readBufIndex = 0;
                needsFseek = false;
            }
        }
    } else {
        uchar currEntry[this->ENTRY_LEN()];
        utils::benchmark::startProfile("fseek");
        std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
        utils::benchmark::stopProfile("fseek");
        this->readEncoded(currEntry);
        bigint positionsChecked = 0;
        while (std::memcmp(currEntry, match, matchLen) != 0) {
            positionsChecked++;
            if (positionsChecked == this->getBcktCount()) {
                return false;
            }

            pos = (pos + this->getBcktSize()) % this->capacity;
            if (this->getBcktSize() > 1 || pos < this->getBcktSize()) {
                utils::benchmark::startProfile("fseek");
                std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
                utils::benchmark::stopProfile("fseek");
            }

            this->readEncoded(currEntry);
        }
    }

    return true;
}


bool EncIndBase::readEntry(ubigint pos, EncIndEntry& ret) const {
    pos %= this->capacity;

    uchar entry[this->ENTRY_LEN()];
    utils::benchmark::startProfile("fseek");
    std::fseek(this->file, pos * this->ENTRY_LEN(), SEEK_SET);
    utils::benchmark::stopProfile("fseek");
    this->readEncoded(entry);
    if (std::memcmp(entry, this->NULL_ENTRY, this->ENTRY_LEN()) == 0) {
        // if `pos` contains `this->NULL_ENTRY`
        return false;
    }

    ret = EncIndEntry::fromUcstr(entry, this->KEY_LEN(), this->DATA_LEN(), utils::crypto::IV_LEN);
    return true;
};


bigint EncIndBase::readIntoReadBuf(
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
            std::cerr << "Error: EncIndBase::readIntoReadBuf(): error reading (part 1) "
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
                std::cerr << "Error: EncIndBase::writeToFirstEmpty(): error reading (part 2) "
                          << "from file " << this->filename
                          << " (only read " << itemsRead << " out of " << entriesToRead << ")"
                          << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });
    }
    
    return itemsRead;
}
