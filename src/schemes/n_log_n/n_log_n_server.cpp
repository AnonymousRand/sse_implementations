#include "schemes/n_log_n/n_log_n_server.h"

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/types.h"
#include "utils/ustring.h"


namespace {


int64_t calcAllEncIndLvlsBytes(const std::vector<EncInd*>& encIndLvls) {
    int64_t bytes = 0;
    for (EncInd* encIndLvl : encIndLvls) {
        bytes += encIndLvl->getSize() * EncInd::ENTRY_LEN;
    }
    return bytes;
}


} // anonymous namespace


//==============================================================================
// `NLogNServer`
//==============================================================================


template <IsDbTuple DbTuple>
NLogNServer<DbTuple>::~NLogNServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::clear() {
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


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::setEncIndLvls(const std::vector<EncInd*>& encIndLvls) {
    int64_t allEncIndLvlsBytes = ::calcAllEncIndLvlsBytes(encIndLvls);
    this->benchmark->diskSize += allEncIndLvlsBytes;
    this->benchmark->network += allEncIndLvlsBytes;

    this->encIndLvls = encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncInd*> NLogNServer<DbTuple>::getEncIndLvls() const {
    int64_t allEncIndLvlsBytes = ::calcAllEncIndLvlsBytes(this->encIndLvls);
    this->benchmark->diskSize += allEncIndLvlsBytes;
    this->benchmark->network += allEncIndLvlsBytes;

    return this->encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncIndVal> NLogNServer<DbTuple>::searchEncIndForBckt(
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


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::setDbKwCountsDict(EncInd* dbKwCountsDict) {
    int64_t dbKwCountsDictBytes = dbKwCountsDict->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->diskSize += dbKwCountsDictBytes;
    this->benchmark->network += dbKwCountsDictBytes;
    this->dbKwCountsDict = dbKwCountsDict;
}


template <IsDbTuple DbTuple>
bool NLogNServer<DbTuple>::getDbKwCount(
    uint64_t pos, const ustring& label, EncIndVal& ret
) const {
    this->benchmark->network += sizeof(uint64_t) + label.length() + EncInd::VAL_LEN;
    return this->dbKwCountsDict->find(pos, label, ret);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNServer<Tuple<>>;
template class NLogNServer<SrcIDb1Tuple>;
//template class NLogNServer<Tuple<IdAlias>;
