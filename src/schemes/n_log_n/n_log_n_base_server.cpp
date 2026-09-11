#include "schemes/n_log_n/n_log_n_base_server.h"

#include <concepts>
#include <vector>

#include "schemes/interfaces/i_sse_server.h"

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_loc.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace {


bigint calcAllEncIndLvlsBytes(const std::vector<EncIndLoc*>& encIndLvls) {
    bigint bytes = 0;
    for (EncIndLoc* encIndLvl : encIndLvls) {
        bytes += encIndLvl->getBytes();
    }
    return bytes;
}


} // anonymous namespace


//==============================================================================
// `NLogNBaseServer`
//==============================================================================


template <IsDbTuple DbTuple>
NLogNBaseServer<DbTuple>::~NLogNBaseServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <IsDbTuple DbTuple>
void NLogNBaseServer<DbTuple>::clear() {
    for (EncIndLoc* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            utils::benchmark::serverStorage -= lvl->getBytes();

            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();
}


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
void NLogNBaseServer<DbTuple>::setEncIndLvls(const std::vector<EncIndLoc*>& encIndLvls) {
    bigint allEncIndLvlsBytes = ::calcAllEncIndLvlsBytes(encIndLvls);
    utils::benchmark::serverStorage += allEncIndLvlsBytes;
    utils::benchmark::communication += allEncIndLvlsBytes;

    this->encIndLvls = encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncIndLoc*> NLogNBaseServer<DbTuple>::getEncIndLvls() const {
    utils::benchmark::communication += ::calcAllEncIndLvlsBytes(this->encIndLvls);

    return this->encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncIndVal> NLogNBaseServer<DbTuple>::searchEncIndForBckt(
    bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
) const {
    utils::benchmark::communication +=
        sizeof(bigint) + sizeof(ubigint) + sizeof(bigint) + label.length();

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
            isFound = this->encIndLvls[lvl]->read(startPos + dbKwCounter, encIndVal, false);
        }
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        utils::benchmark::communication += this->encIndLvls[lvl]->VAL_LEN();
    }

    return encResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNBaseServer<Tuple<>>;
template class NLogNBaseServer<SrcIDb1Tuple>;
//template class NLogNBaseServer<Tuple<IdAlias>;
