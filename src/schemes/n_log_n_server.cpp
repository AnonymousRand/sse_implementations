#include "schemes/n_log_n_server.h"

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/doc.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
NLogNServer<DbDoc, DbKw>::~NLogNServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::clear() {
    for (EncInd* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            this->benchmark->diskSize -= lvl->getSize() * EncInd::ENTRY_LEN;
            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();

    if (this->dbKwCountsDict != nullptr) {
        this->benchmark->diskSize -= this->dbKwCountsDict->getSize() * EncInd::ENTRY_LEN;
        delete this->dbKwCountsDict;
        this->dbKwCountsDict = nullptr;
    }
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::setEncIndLvls(const std::vector<EncInd*>& encIndLvls) {
    int64_t encIndBytes = 0;
    for (EncInd* encIndLvl : encIndLvls) {
        encIndBytes += encIndLvl->getSize() * EncInd::ENTRY_LEN;
    }
    this->benchmark->diskSize += encIndBytes;
    this->benchmark->network += encIndBytes;
    this->encIndLvls = encIndLvls;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<EncInd*> NLogNServer<DbDoc, DbKw>::getEncIndLvls() const {
    if (encIndLvls.size() > 0) {
        this->benchmark->network +=
            encIndLvls.size() * encIndLvls[0]->getSize() * EncInd::ENTRY_LEN;
    }
    return this->encIndLvls;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<EncIndVal> NLogNServer<DbDoc, DbKw>::searchEncIndForBckt(
    int64_t lvl, uint64_t startPos, int64_t bcktSize, const ustring& label
) const {
    this->benchmark->network +=
        sizeof(int64_t) + sizeof(uint64_t) + sizeof(int64_t) + label.length();
    std::vector<EncIndVal> encResults;

    for (int64_t dbKwCounter = 0; dbKwCounter < bcktSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of modulo
            // or hash collision in encrypted index)
            // note: dummies must also use the correct (not dummy) `label` so they
            // are still found by `find()`
            isFound = this->encIndLvls[lvl]->find(startPos, label, encIndVal, &startPos);
        } else {
            // after first read, just read from the bucket consecutively as we are
            // now guaranteed consecutivity
            isFound = this->encIndLvls[lvl]->read(startPos + dbKwCounter, encIndVal);
        }
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
    }

    this->benchmark->network += encResults.size() * EncInd::VAL_LEN;
    return encResults;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogNServer<DbDoc, DbKw>::setDbKwCountsDict(EncInd* dbKwCountsDict) {
    int64_t dbKwCountsDictBytes = dbKwCountsDict->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->diskSize += dbKwCountsDictBytes;
    this->benchmark->network += dbKwCountsDictBytes;
    this->dbKwCountsDict = dbKwCountsDict;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
bool NLogNServer<DbDoc, DbKw>::getDbKwCount(
    uint64_t pos, const ustring& label, EncIndVal& ret
) const {
    this->benchmark->network += sizeof(uint64_t) + label.length() + EncInd::VAL_LEN;
    return this->dbKwCountsDict->find(pos, label, ret);
}


template class NLogNServer<Doc<>, Kw>;
template class NLogNServer<SrcIDb1Doc, Kw>;
//template class NLogNServer<Doc<IdAlias>, IdAlias>;
