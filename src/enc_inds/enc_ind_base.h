/**
 * Indexes are abstractly a ccllection of `std::pair<ustring, std::pair<ustring, ustring>>` pairs,
 * each of which correspond to `std::pair<key/label, std::pair<encrypted doc, IV>>`.
 */


#pragma once


#include <cstdio>

#include "schemes/sse.h"
#include "utils/utils.h"


class EncIndBase {
    public:
        static constexpr int KEY_LEN = HASH_OUTPUT_LEN; // both PRF (default) and hash (res-hiding) have 512 bit output
        static constexpr int DOC_LEN = 4 * BLOCK_SIZE;  // so max keyword/id size ~10^13 for encoding to fit (start 0)
        static constexpr int VAL_LEN = EncIndBase::DOC_LEN + IV_LEN;
        static constexpr int KV_LEN  = EncIndBase::KEY_LEN + EncIndBase::VAL_LEN;

        virtual ~EncIndBase();

        virtual void init(long size) = 0;
        virtual void clear() = 0;

    protected:
        static const uchar NULL_KV[EncIndBase::KV_LEN];

        FILE* file = nullptr;
        std::string filename = "";
        long size;

        /**
         * Returns:
         *     - `true` if the kv pair at `pos` is valid.
         *     - `false` if the kv pair at `pos` is the null kv pair.
         */
        bool readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const;
        void writeToPos(ulong pos, const EncIndEntry& encIndEntry, bool flushImmediately = false);
};
