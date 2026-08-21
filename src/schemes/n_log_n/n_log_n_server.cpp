#include "schemes/n_log_n/n_log_n_server.h"

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace {


bigint calcAllEncIndLvlsBytes(const std::vector<EncIndLoc*>& encIndLvls) {
    bigint bytes = 0;
    for (EncIndLoc* encIndLvl : encIndLvls) {
        bytes += encIndLvl->getSize() * EncIndBase::ENTRY_LEN;
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
    for (EncIndLoc* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            this->benchmark->serverStorage -= lvl->getSize() * EncIndBase::ENTRY_LEN;
            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();

    if (this->dbKwCountsDict != nullptr) {
        this->benchmark->serverStorage -= this->dbKwCountsDict->getSize() * EncIndBase::ENTRY_LEN;
        delete this->dbKwCountsDict;
        this->dbKwCountsDict = nullptr;
    }
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::setEncIndLvls(const std::vector<EncIndLoc*>& encIndLvls) {
    bigint allEncIndLvlsBytes = ::calcAllEncIndLvlsBytes(encIndLvls);
    this->benchmark->serverStorage += allEncIndLvlsBytes;
    this->benchmark->communication += allEncIndLvlsBytes;

    this->encIndLvls = encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncIndLoc*> NLogNServer<DbTuple>::getEncIndLvls() const {
    bigint allEncIndLvlsBytes = ::calcAllEncIndLvlsBytes(this->encIndLvls);
    this->benchmark->serverStorage += allEncIndLvlsBytes;
    this->benchmark->communication += allEncIndLvlsBytes;

    return this->encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncIndVal> NLogNServer<DbTuple>::searchEncIndForBckt(
    bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
) const {
    this->benchmark->communication +=
        sizeof(bigint) + sizeof(ubigint) + sizeof(bigint) + sizeof(bigint) + label.length();
    std::vector<EncIndVal> encResults;

    for (bigint dbKwCounter = 0; dbKwCounter < bcktSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of modulo
            // collision in encrypted index)
            // (note: dummies must also use the correct (not dummy) `label` so they
            // are still found by `find()`)
            isFound = this->encIndLvls[lvl]->find(startPos, label, encIndVal);
        } else {
            // after first read, just read from the bucket consecutively as we are
            // now guaranteed that the full bucket is stored here contiguously
            isFound = this->encIndLvls[lvl]->read(startPos + dbKwCounter, encIndVal);
        }
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
    }

    this->benchmark->communication += encResults.size() * EncIndBase::VAL_LEN;
    return encResults;
}


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::setDbKwCountsDict(EncIndRand* dbKwCountsDict) {
    bigint dbKwCountsDictBytes = dbKwCountsDict->getSize() * EncIndBase::ENTRY_LEN;
    this->benchmark->serverStorage += dbKwCountsDictBytes;
    this->benchmark->communication += dbKwCountsDictBytes;
    this->dbKwCountsDict = dbKwCountsDict;
}


template <IsDbTuple DbTuple>
bool NLogNServer<DbTuple>::getDbKwCount(
    ubigint pos, const ustring& label, EncIndVal& ret
) const {
    this->benchmark->communication += sizeof(ubigint) + label.length() + EncIndBase::VAL_LEN;
    return this->dbKwCountsDict->find(pos, label, ret);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNServer<Tuple<>>;
template class NLogNServer<SrcIDb1Tuple>;
//template class NLogNServer<Tuple<IdAlias>;
