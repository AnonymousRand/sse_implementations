#include "enc_ind_nlogn.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndNlogn<DbDoc, DbKw>::init(long size) {
    // log N blowup factor for storage
    long numLevels = std::ceil(std::log2(size)) + 1;
    this->levels.reserve(numLevels);
    for (long i = 0; i < numLevels; i++) {
        EncIndPibas<DbDoc, DbKw>* level = new EncIndPibas<DbKw>();
        level->init(size)
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndNlogn<DbDoc, DbKw>::clear() {
    for (EncIndPibas<DbDoc, DbKw>* level : this->levels) {
        if (level != nullptr) {
            delete level;
            level = nullptr;
        }
    }
    this->levels.clear();
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndNlogn<DbDoc, DbKw>::write(
    const ustring& key, const std::pair<ustring, ustring>& val,
    const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize
) {
    ulong pos = this->map(dbKwRange, dbKwCount, dbKwCounter, minDbKw, bottomLevelSize);
    this->writeToPos(pos, key, val);
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void EncIndNlogn<DbDoc, DbKw>::find(
    const Range<DbKw>& dbKwRange, long dbKwCount, long dbKwCounter, DbKw minDbKw, long bottomLevelSize,
    std::pair<ustring, ustring>& ret
) const {
    ulong pos = this->map(dbKwRange, dbKwCount, dbKwCounter, minDbKw, bottomLevelSize);
    this->readValFromPos(pos, ret);
}


template class EncIndNlogn<Kw>;
//template class EncIndNlogn<IdAlias>;
