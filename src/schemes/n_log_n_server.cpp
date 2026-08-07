#include "schemes/n_log_n_server.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
NLogNServer<DbDoc, DbKw>::~NLogNServer() {
    this->clear();
    if (this->dbKwCountsDict != nullptr) {
        delete this->dbKwCountsDict;
        this->dbKwCountsDict = nullptr;
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::clear() {
    for (EncInd* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();

    if (this->dbKwCountsDict != nullptr) {
        this->dbKwCountsDict->clear();
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::addEncIndLvl(EncInd* encIndLvl) {
    this->benchmark.communication += encIndLvl->getSize() * EncInd::ENTRY_LEN;
    this->encIndLvls.push_back(encIndLvl);
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::writeToEncInd(int64_t lvl, uint64_t pos, const EncIndEntry& entry) {
    this->benchmark.communication += EncInd::VAL_LEN;
    //std::cerr << pos << ": " << strToHex(toUstr(entry)) << std::endl;
    this->encIndLvls[lvl]->write(pos, entry);
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
std::vector<EncIndVal> NLogNServer<DbDoc, DbKw>::findEncIndBucket(
    int64_t lvl, uint64_t startPos, int64_t bucketSize, const ustring& label
) const {
    std::vector<EncIndVal> encResults;

    for (int64_t dbKwCounter = 0; dbKwCounter < bucketSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of hash/modulo collision in encrypted index)
            // note: dummies must also use the correct (not dummy) `label` so they are still found by `find()`
            isFound = this->encIndLvls[lvl]->find(startPos, label, encIndVal, &startPos);
        } else {
            // after first read, just read from the bucket consecutively as we are now guaranteed consecutivity
            isFound = this->encIndLvls[lvl]->read(startPos + dbKwCounter, encIndVal);
        }
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        this->benchmark.communication += EncInd::VAL_LEN;
    }

    return encResults;
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
bool NLogNServer<DbDoc, DbKw>::getEncIndVal(int64_t lvl, uint64_t pos, EncIndVal& ret) const {
    this->benchmark.communication += EncInd::VAL_LEN;
    return this->encIndLvls[lvl]->read(pos, ret);
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::initDbKwCountsDict(int64_t size) {
    this->dbKwCountsDict->init(size);
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::writeToDbKwCountsDict(uint64_t pos, const EncIndEntry& entry) {
    this->benchmark.communication += EncInd::ENTRY_LEN;
    this->dbKwCountsDict->write(pos, entry);
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
bool NLogNServer<DbDoc, DbKw>::getDbKwCount(uint64_t pos, const ustring& label, EncIndVal& ret) {
    this->benchmark.communication += label.length() + EncInd::VAL_LEN;
    return this->dbKwCountsDict->find(pos, label, ret);
}


template class NLogNServer<Doc<>, Kw>;
template class NLogNServer<SrcIDb1Doc, Kw>;
//template class NLogNServer<Doc<IdAlias>, IdAlias>;
