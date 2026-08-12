#include "schemes/n_log_n/n_log_n_server.h"

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
NLogNServer<DbRecord, DbKw>::~NLogNServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void NLogNServer<DbRecord, DbKw>::clear() {
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


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void NLogNServer<DbRecord, DbKw>::setEncIndLvls(const std::vector<EncInd*>& encIndLvls) {
    int64_t encIndBytes = 0;
    for (EncInd* encIndLvl : encIndLvls) {
        encIndBytes += encIndLvl->getSize() * EncInd::ENTRY_LEN;
    }
    this->benchmark->diskSize += encIndBytes;
    this->benchmark->network += encIndBytes;
    this->encIndLvls = encIndLvls;
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
std::vector<EncInd*> NLogNServer<DbRecord, DbKw>::getEncIndLvls() const {
    if (encIndLvls.size() > 0) {
        this->benchmark->network +=
            encIndLvls.size() * encIndLvls[0]->getSize() * EncInd::ENTRY_LEN;
    }
    return this->encIndLvls;
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
std::vector<EncIndVal> NLogNServer<DbRecord, DbKw>::searchEncIndForBckt(
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


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void NLogNServer<DbRecord, DbKw>::setDbKwCountsDict(EncInd* dbKwCountsDict) {
    int64_t dbKwCountsDictBytes = dbKwCountsDict->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->diskSize += dbKwCountsDictBytes;
    this->benchmark->network += dbKwCountsDictBytes;
    this->dbKwCountsDict = dbKwCountsDict;
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
bool NLogNServer<DbRecord, DbKw>::getDbKwCount(
    uint64_t pos, const ustring& label, EncIndVal& ret
) const {
    this->benchmark->network += sizeof(uint64_t) + label.length() + EncInd::VAL_LEN;
    return this->dbKwCountsDict->find(pos, label, ret);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNServer<Record<>, Kw>;
template class NLogNServer<SrcIDb1Record, Kw>;
//template class NLogNServer<Record<IdAlias>, IdAlias>;
