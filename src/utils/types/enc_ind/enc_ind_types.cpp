#include "utils/types/enc_ind/enc_ind_types.h"

#include "utils/types/ustring.h"


//==============================================================================
// `EncIndVal`
//==============================================================================


ustring EncIndVal::encode() const {
    return this->data + this->iv;
}


EncIndVal EncIndVal::decode(const uchar* encoding, int dataLen, int ivLen) {
    ustring data(encoding, dataLen);
    ustring iv(encoding + dataLen, ivLen);
    return EncIndVal {data, iv};
}


//==============================================================================
// `EncIndEntry`
//==============================================================================


ustring EncIndEntry::encode() const {
    return this->key + this->val.encode();
}


EncIndEntry EncIndEntry::decode(const uchar* encoding, int keyLen, int dataLen, int ivLen) {
    ustring key(encoding, keyLen);
    EncIndVal encIndVal = EncIndVal::decode(encoding + keyLen, dataLen, ivLen);
    return EncIndEntry {key, encIndVal};
}
