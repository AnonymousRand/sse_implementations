#include "utils/types/encr_ind/encr_ind_types.h"

#include "utils/types/ustring.h"


//==============================================================================
// `EncrIndVal`
//==============================================================================


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
    EncrIndVal encIndVal = EncrIndVal::decode(encoding + keyLen, dataLen, ivLen);
    return EncrIndEntry {key, encIndVal};
}
