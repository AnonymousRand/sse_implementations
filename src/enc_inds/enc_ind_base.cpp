#include "enc_ind_base.h"


// this initializes everything to `\0`, i.e. zero bits
// technically it is possible that some encrypted tuple happened to be all `0` bytes and thus get mistaken for
// a null kv-pair, but currently `EncIndBase::KV_LEN` is 1024 bits so there's a 2^1024 chance of this happening
// USENIX'24's implementation also seems to just do this
const uchar EncIndBase::NULL_KV[EncIndBase::KV_LEN] = {};


EncIndBase::~EncIndBase() {
    this->clear();
}


bool EncIndBase::readValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    uchar kv[EncIndBase::KV_LEN];
    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsRead = std::fread(kv, EncIndBase::KV_LEN, 1, this->file);
    if (itemsRead != 1) {
        std::cerr << "EncIndBase::readValFromPos(): error reading from file (nothing read)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(kv, EncIndBase::NULL_KV, EncIndBase::KV_LEN) == 0) {
        // if `pos` contains `NULL_KV`
        return false;
    }

    ret.first = ustring(&kv[EncIndBase::KEY_LEN], EncIndBase::DOC_LEN);
    ret.second = ustring(&kv[EncIndBase::KEY_LEN + EncIndBase::DOC_LEN], IV_LEN);
    return true;
}


void EncIndBase::writeToPos(ulong pos, const EncIndEntry& encIndEntry, bool flushImmediately) {
    if (pos >= this->size) {
        std::cerr << "EncIndBase::writeToPos(): write to pos " << pos << " is out of bounds! "
                  << "(size is " << this->size << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ustring key = encIndEntry.first;
    std::pair<ustring, ustring> val = encIndEntry.second;
    ustring kv = key + val.first + val.second;
    if (kv.length() != EncIndBase::KV_LEN) {
        std::cerr << "EncIndBase::writeToPos(): write of length " << kv.length() << " bytes is not allowed! "
                  << "(want " << EncIndBase::KV_LEN << " bytes)" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::fseek(this->file, pos * EncIndBase::KV_LEN, SEEK_SET);
    int itemsWritten = std::fwrite(kv.c_str(), EncIndBase::KV_LEN, 1, this->file);
    if (itemsWritten != 1) {
        std::cerr << "EncIndBase::writeToPos(): error writing to file (nothing written)" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (flushImmediately) {
        std::fflush(this->file);
    }
}
