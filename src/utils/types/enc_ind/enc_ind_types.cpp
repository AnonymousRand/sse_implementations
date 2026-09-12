#include "utils/types/enc_ind/enc_ind_types.h"

#include "utils/types/ustring.h"


//==============================================================================
// `EncIndVal`
//==============================================================================


ustring EncIndVal::toUstr() const {
    return this->data + this->iv;
}


EncIndVal EncIndVal::fromUcstr(const uchar* ucstr, int dataLen, int ivLen) {
    ustring data(ucstr, dataLen);
    ustring iv(ucstr + dataLen, ivLen);
    return EncIndVal {data, iv};
}


//==============================================================================
// `EncIndEntry`
//==============================================================================


ustring EncIndEntry::toUstr() const {
    return this->key + this->val.toUstr();
}


EncIndEntry EncIndEntry::fromUcstr(const uchar* ucstr, int keyLen, int dataLen, int ivLen) {
    ustring key(ucstr, keyLen);
    EncIndVal encIndVal = EncIndVal::fromUcstr(ucstr + keyLen, dataLen, ivLen);
    return EncIndEntry {key, encIndVal};
}
