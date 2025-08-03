/**
 * Indexes are abstractly a list of `std::pair<ustring, std::pair<ustring, ustring>>` entries,
 * each of which correspond to `std::pair<key/label, std::pair<encrypted doc, IV>>`.
 */


#pragma once


#include <cstdio>

#include "utils.h"


/******************************************************************************/
/* `EncIndBase`                                                               */
/******************************************************************************/


// common code
class EncIndBase {
    public:
        static constexpr int KEY_LEN = HASH_OUTPUT_LEN; // both PRF (default) and hash (res-hiding) have 512 bit output
        static constexpr int DOC_LEN = 4 * BLOCK_SIZE;  // so max keyword/id size ~10^13 for encoding to fit (start 0)
        static constexpr int VAL_LEN = EncIndBase::DOC_LEN + IV_LEN;
        static constexpr int KV_LEN  = EncIndBase::KEY_LEN + EncIndBase::VAL_LEN;

        virtual ~EncIndBase();

        virtual void init(long size);
        void clear();

        /**
         * Returns:
         *     - `true` if the kv pair at `pos` is valid.
         *     - `false` if the kv pair at `pos` is the null kv pair.
         */
        bool readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;

    protected:
        static const uchar NULL_KV[EncIndBase::KV_LEN];

        FILE* file = nullptr;
        std::string filename = "";
        long size;

        void writeToPos(
            ulong pos, const ustring& key, const std::pair<ustring, ustring>& val, bool flushImmediately = false
        );
};


/******************************************************************************/
/* `EncInd`                                                                   */
/******************************************************************************/


class EncInd : public EncIndBase {
    public:
        void write(const ustring& key, const std::pair<ustring, ustring>& val);

        /**
         * Returns:
         *     - `true` if `key` found.
         *     - `false` if `key` not found.
         */
        bool find(const ustring& key, std::pair<ustring, ustring>& ret) const;
};


/******************************************************************************/
/* `EncIndLoc`                                                                */
/******************************************************************************/


template <class DbKw>
class EncIndLoc : public EncIndBase {
    public:
        void write(
            const ustring& key, const std::pair<ustring, ustring>& val,
            const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
        );
        void find(
            const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize,
            std::pair<ustring, ustring>& ret
        ) const;

        /**
         * Returns the position in the file/index that the given keyword range goes (with Log-SRC-i* locality).
         *
         * Precondition:
         *     - `dbKwCount` is a power of 2.
         *     - `bottomLevelSize` is a power of 2.
         */
        static ulong map(
            const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
        );
};
