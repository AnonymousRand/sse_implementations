#pragma once

#include "utils/types/ustring.h"


//==============================================================================
// `EncIndVal`
//==============================================================================


/**
 * encrypted indexes are a collection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncIndEntry`) pairs, corresponding to `std::pair<key, std::pair<encrypted data, IV>>`.
 */
struct EncIndVal {
    ustring data;
    ustring iv;

    ustring toUstr() const;
    static EncIndVal fromUcstr(const uchar* ucstr, int dataLen, int ivLen);
};


//==============================================================================
// `EncIndEntry`
//==============================================================================


struct EncIndEntry {
    ustring key;
    EncIndVal val;

    ustring toUstr() const;
    static EncIndEntry fromUcstr(const uchar* ucstr, int keyLen, int dataLen, int ivLen);
};
