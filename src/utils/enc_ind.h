/**
 * indexes are abstractly a ccllection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncIndVal`) pairs,
 * each of which correspond to `std::pair<key/label, std::pair<encrypted record, IV>>`.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

#include "utils/crypto.h"
#include "utils/ustring.h"


//==============================================================================
// utils
//==============================================================================


using EncIndVal   = std::pair<ustring, ustring>;
using EncIndRecord = std::pair<ustring, EncIndVal>;


namespace utils {


ustring toUstr(const EncIndRecord& encIndRecord);


} // namespace `utils`


//==============================================================================
// `EncInd`
//==============================================================================


class EncInd {
public:
    // (both PRF (default) and hash (res-hiding) have 512 bit output)
    static constexpr int KEY_LEN    = utils::HASH_OUTPUT_LEN;
    // (so max keyword/id size ~1.8x10^16 for encoding to fit)
    static constexpr int DOC_LEN    = 7 * utils::BLOCK_SIZE;
    static constexpr int VAL_LEN    = EncInd::DOC_LEN + utils::IV_LEN;
    static constexpr int ENTRY_LEN  = EncInd::KEY_LEN + EncInd::VAL_LEN;

    ~EncInd();

    void init(int64_t size);
    void clear();

    /**
     * tries to find `key` starting at `pos` and iterating linearly if not matching
     * (i.e. another kv pair overflowed there first).
     *
     * params:
     *     - `posFoundAt`: pointer whose value is replaced by the `pos` at which `key`
     *       was eventually found (if it was found; otherwise it is left unchanged).
     *       set to `nullptr` to not receive this value.
     *
     * returns:
     *     - `true` if the kv pair corresponding to `key` was eventually found.
     *     - `false` if the kv pair corresponding to `key` was never found in the entire index.
     */
    bool find(
        uint64_t pos, const ustring& key, EncIndVal& ret, uint64_t* posFoundAt = nullptr
    ) const;

    /**
     * reads and decodes the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the kv pair at `pos` is valid.
     *     - `false` if the kv pair at `pos` is the null kv pair.
     */
    bool read(uint64_t pos, EncIndVal& ret) const;

    /**
     * write to first empty location starting at `pos` (may not be at `pos` if hash collision).
     */
    void write(uint64_t pos, const EncIndRecord& encIndRecord);

    int64_t getSize() const;

private:
    static const uchar NULL_ENTRY[EncInd::ENTRY_LEN];

    FILE* file = nullptr;
    std::string filename = "";
    int64_t size;

    //----------------------------------------------------------------------
    // debugging

    EncIndRecord get(uint64_t pos) const;
    void print() const; // warning: this can be, like, a LOT of stuff!!
};
