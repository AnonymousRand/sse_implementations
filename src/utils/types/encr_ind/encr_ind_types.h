#pragma once

#include "utils/types/ustring.h"


//==============================================================================
// `EncrIndVal`
//==============================================================================


/**
 * encrypted indexes are a collection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncrIndEntry`) pairs, corresponding to `std::pair<key, std::pair<encrypted data, IV>>`.
 */
struct EncrIndVal {
    ustring data;
    ustring iv;

    ustring encode() const;
    static EncrIndVal decode(const uchar* encoding, int dataLen, int ivLen);
};


//==============================================================================
// `EncrIndEntry`
//==============================================================================


struct EncrIndEntry {
    ustring key;
    EncrIndVal val;

    ustring encode() const;
    static EncrIndEntry decode(const uchar* encoding, int keyLen, int dataLen, int ivLen);
};
