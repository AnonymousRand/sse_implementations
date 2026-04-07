/**
 * Indexes are abstractly a ccllection of `std::pair<ustring, std::pair<ustring, ustring>>` (aka `EncIndVal`) pairs,
 * each of which correspond to `std::pair<key/label, std::pair<encrypted doc, IV>>`.
 */


#pragma once


#include <cstdio>

#include "utils.h"


class EncInd {
    public:
        static constexpr int KEY_LEN = HASH_OUTPUT_LEN; // both PRF (default) and hash (res-hiding) have 512 bit output
        static constexpr int DOC_LEN = 4 * BLOCK_SIZE;  // so max keyword/id size ~10^13 for encoding to fit (start 0)
        static constexpr int VAL_LEN = EncInd::DOC_LEN + IV_LEN;
        static constexpr int KV_LEN  = EncInd::KEY_LEN + EncInd::VAL_LEN;

        ~EncInd();

        void init(long size);
        void clear();

        /**
         * Tries to find `key` starting at `pos` and iterating linearly if not matching
         * (i.e. another kv pair overflowed there first).
         *
         * Params:
         *     - `posFoundAt`: pointer whose value is replaced by the `pos` at which `key` was eventually found
         *       (if it was found; otherwise it is left unchanged). Set to `nullptr` to not receive this value.
         *
         * Returns:
         *     - `true` if the kv pair corresponding to `key` was eventually found.
         *     - `false` if the kv pair corresponding to `key` was never found in the entire index.
         */
        bool find(ulong pos, const ustring& key, EncIndVal& ret, ulong* posFoundAt = nullptr) const;

        /**
         * Reads and decodes the value at `pos` (without checking the "key`).
         *
         * Returns:
         *     - `true` if the kv pair at `pos` is valid.
         *     - `false` if the kv pair at `pos` is the null kv pair.
         */
        bool read(ulong pos, EncIndVal& ret) const;

        /**
         * Write to first empty location starting at `pos` (may not be at `pos` if hash collision).
         */
        void write(ulong pos, const EncIndEntry& encIndEntry);
        long getSize() const;

    protected:
        static const uchar NULL_KV[EncInd::KV_LEN];

        FILE* file = nullptr;
        std::string filename = "";
        long size;
};
