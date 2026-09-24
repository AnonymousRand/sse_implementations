#include "utils/types/enc_ind/enc_ind_types.h"

#include <cstring>

#include "utils/types/ustring.h"


//==============================================================================
// `EncIndVal`
//==============================================================================


void EncIndVal::encode(uchar* ret, int dataLen, int ivLen) const {
    std::memcpy(ret, this->data, dataLen);
    std::memcpy(ret + dataLen, this->iv, ivLen);
}


EncIndVal EncIndVal::decode(const uchar* encoding, int dataLen, int ivLen) {
    uchar* data = new uchar[dataLen];
    uchar* iv = new uchar[dataLen];data(encoding, dataLen);
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
