#include "utils/types/enc_ind/enc_ind_types.h"

#include "utils/types/ustring.h"


ustring EncIndEntry::toUstr() const {
    return this->key + this->val.data + this->val.iv;
}


EncIndEntry EncIndEntry::fromUcstr(const uchar* ucstr, int keyLen, int dataLen, int ivLen) {
    ustring key(&ucstr[0], keyLen);
    ustring data(&ucstr[keyLen], dataLen);
    ustring iv(&ucstr[keyLen + dataLen], ivLen);
    return EncIndEntry {key, EncIndVal {data, iv}};
}
