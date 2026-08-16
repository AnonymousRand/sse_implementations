/**
 * indexes are abstractly a collection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncIndEntry`) pairs, corresponding to `std::pair<key, std::pair<encrypted data, IV>>`.
 */

#pragma once

#include <cmath>
#include <string>
#include <utility>

#include "config.h"

#include "utils/crypto.h"
#include "utils/disk_storage.h"
#include "utils/types.h"
#include "utils/ustring.h"


//==============================================================================
// utils
//==============================================================================


using EncIndVal   = std::pair<ustring, ustring>;
using EncIndEntry = std::pair<ustring, EncIndVal>;


namespace utils {


ustring toUstr(const EncIndEntry& encIndEntry);


} // namespace `utils`


//==============================================================================
// `EncInd`
//==============================================================================


class EncInd : public IDiskStorage {
public:
    // (both PRF (default) and hash (res-hiding) have 512 bit output)
    inline static constexpr int KEY_LEN   = crypto::HASH_OUTPUT_LEN;
    // (currently, encoding a `Tuple<>` is of the form `(id,kw,op),dbKw-dbKw` (and encrypting an
    // exactly n block length plaintext with AES should produce the exact same block ciphertext),
    // so all but 7 bytes are divided up between `id`, `kw`, and 2 `dbKw`s. however, we actually
    // must restrict our plaintexts by one more byte or else AES' PCKS #7 padding will generate
    // an extra block if our plaintext is exactly an integer number of blocks long, thus the `+8`.)
    // also round up to the next AES block
    inline static const int     DATA_LEN  = std::ceil(
        (4 * config::MAX_VALUE_DIGITS + 8) / (float)crypto::BLOCK_SIZE
    ) * crypto::BLOCK_SIZE;
    inline static constexpr int VAL_LEN   = DATA_LEN + crypto::IV_LEN;
    inline static constexpr int ENTRY_LEN = KEY_LEN + VAL_LEN;

    //--------------------------------------------------------------------------
    // constructors/destructors

    EncInd() = default;

    //--------------------------------------------------------------------------
    // copy/move

    // these are still manually written even though all of them are ` = default` because the
    // parent `IDb`'s manually declared move assignment operator prevents compiler from
    // automatically generating all of these
    //
    // the default behavior should call the parent(s)' version(s) before doing a per-member
    // copy/move of the child's members

    // copy constructor
    EncInd(const EncInd& other) = default;

    // copy assignment operator
    EncInd& operator =(const EncInd& other) = default;

    // move constructor
    EncInd(EncInd&& other) noexcept = default;

    // move assignment operator
    EncInd& operator =(EncInd&& other) noexcept = default;

    //--------------------------------------------------------------------------
    // `IDiskStorage`

    void init(bigint size);
    void clear() override;

    //--------------------------------------------------------------------------
    // other interface

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
        ubigint pos, const ustring& key, EncIndVal& ret, ubigint* posFoundAt = nullptr
    ) const;

    /**
     * reads and decodes the value at `pos` (without checking the "key`).
     *
     * returns:
     *     - `true` if the kv pair at `pos` is valid.
     *     - `false` if the kv pair at `pos` is the null kv pair.
     */
    bool read(ubigint pos, EncIndVal& ret) const;

    /**
     * write to first empty location starting at `pos` (may not be at `pos` if hash collision).
     */
    void write(ubigint pos, const EncIndEntry& encIndEntry);

    bigint getSize() const;

private:
    constexpr std::string FILE_DIR() const override { return "out/server"; }
    constexpr std::string FILENAME_PREFIX() const override { return "enc_ind_"; }

    static const uchar NULL_ENTRY[ENTRY_LEN];

    bigint size = 0;

    //----------------------------------------------------------------------
    // debugging

    EncIndEntry get(ubigint pos) const;
    void print() const; // warning: this can be, like, a LOT of stuff!!
};
