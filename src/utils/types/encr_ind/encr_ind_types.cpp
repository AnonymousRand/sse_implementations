#include "utils/types/encr_ind/encr_ind_types.h"

#include "utils/types/ustring.h"


//==============================================================================
// `EncrIndVal`
//==============================================================================


// IMPORTANT: this requires that `this->data` is already padded to the full `dataLen`, as we read/
// decode at fixed indexes, which is why we pad pre-encryption to generate the same # of blocks!
ustring EncrIndVal::encode() const {
    return this->data + this->iv;
}


EncrIndVal EncrIndVal::decode(const uchar* encoding, int dataLen, int ivLen) {
    ustring data(encoding, dataLen);
    ustring iv(encoding + dataLen, ivLen);
    return EncrIndVal {data, iv};
}


//==============================================================================
// `EncrIndEntry`
//==============================================================================


ustring EncrIndEntry::encode() const {
    return this->key + this->val.encode();
}


EncrIndEntry EncrIndEntry::decode(const uchar* encoding, int keyLen, int dataLen, int ivLen) {
    ustring key(encoding, keyLen);
    EncrIndVal encrIndVal = EncrIndVal::decode(encoding + keyLen, dataLen, ivLen);
    return EncrIndEntry {key, encrIndVal};
}
