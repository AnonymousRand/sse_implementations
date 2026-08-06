#include "n_log_n_server.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
NLogNServer<DbDoc, DbKw>::~NLogNServer() {
    this->clear();
    if (this->dbKwListSizeDict != nullptr) {
        delete this->dbKwListSizeDict;
        this->dbKwListSizeDict = nullptr;
    }
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::clear() {
    for (EncInd* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();

    if (this->dbKwListSizeDict != nullptr) {
        this->dbKwListSizeDict->clear();
    }
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::addEncIndLvl(EncInd* encIndLvl) {
    this->encIndLvls.push_back(encIndLvl);
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::initDbKwListSizeDict(long size) {
    this->dbKwListSizeDict->init(size);
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void NLogNServer<DbDoc, DBKw>::writeToEncInd(long lvl, ulong pos, const EncIndEntry& entry) {
    this->encIndLvls[lvl]->write(pos, entry);
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
std::vector<EncIndVal> getBucket(long lvl, ulong pos, const ustring& label) const {
    std::vector<EncIndVal> encResults;

    for (long dbKwCounter = 0; dbKwCounter < dbKwListPaddedSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of hash/modulo collision in encrypted index)
            // note: dummies must also use the correct (not dummy) `label` so they are still found by `find()`
            isFound = this->encIndLvls[lvl]->find(pos, label, encIndVal, &pos);
        } else {
            // after first read, just read from the bucket consecutively as we are now guaranteed consecutivity
            isFound = this->encIndLvls[lvl]->read(pos + dbKwCounter, encIndVal);
        }
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
    }

    return encResults;
}
