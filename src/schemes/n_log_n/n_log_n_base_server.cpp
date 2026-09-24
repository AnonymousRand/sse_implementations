#include "schemes/n_log_n/n_log_n_base_server.h"

#include <cassert>
#include <concepts>
#include <utility>
#include <vector>

#include "schemes/interfaces/i_sse_server.h"

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_loc.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


namespace {


bigint calcAllEncrIndLvlsBytes(const std::vector<EncrIndLoc*>& encrIndLvls) {
    bigint bytes = 0;
    for (EncrIndLoc* encrIndLvl : encrIndLvls) {
        bytes += encrIndLvl->getBytes();
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
    for (EncrIndLoc* lvl : this->encrIndLvls) {
        if (lvl != nullptr) {
            utils::benchmark::serverStorage -= lvl->getBytes();

            delete lvl;
            lvl = nullptr;
        }
    }
    this->encrIndLvls.clear();
}


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
void NLogNBaseServer<DbTuple>::setEncrIndLvls(const std::vector<EncrIndLoc*>& encrIndLvls) {
    bigint allEncrIndLvlsBytes = ::calcAllEncrIndLvlsBytes(encrIndLvls);
    utils::benchmark::serverStorage += allEncrIndLvlsBytes;
    utils::benchmark::communication += allEncrIndLvlsBytes;

    this->encrIndLvls = encrIndLvls;
}


template <IsDbTuple DbTuple>
const std::vector<EncrIndLoc*>& NLogNBaseServer<DbTuple>::getEncrIndLvls() const {
    utils::benchmark::communication += ::calcAllEncrIndLvlsBytes(this->encrIndLvls);

    return this->encrIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncrIndVal> NLogNBaseServer<DbTuple>::searchEncrIndForBckt(
    bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
) const {
    assert(lvl < this->encrIndLvls.size() || this->encrIndLvls.size() == 0);
    utils::benchmark::communication +=
        sizeof(bigint) + sizeof(ubigint) + sizeof(bigint) + label.length();

    std::vector<EncrIndVal> encrResults;
    for (bigint dbKwCounter = 0; dbKwCounter < bcktSize; dbKwCounter++) {
        EncrIndVal encrIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of modulo
            // collision in encrypted index)
            // (NOTE: dummies must also use the correct (not dummy) `label` so they
            // are still found by `find()`!)
            isFound = this->encrIndLvls[lvl]->find(
                SseOper::SEARCH, startPos, label, encrIndVal
            );
        } else {
            // after first read, just read from the bucket consecutively as we are
            // now guaranteed that the full bucket is stored here contiguously
            // 
            // we also stop `fseek()`ing at every read since the read itself should advance
            // the file pointer to the right location for the next one
            isFound = this->encrIndLvls[lvl]->read(
                SseOper::SEARCH, startPos + dbKwCounter, encrIndVal, false
            );
        }
        if (!isFound) {
            break;
        }

        encrResults.emplace_back(std::move(encrIndVal));
        utils::benchmark::communication += this->encrIndLvls[lvl]->VAL_LEN();
    }

    return encrResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNBaseServer<Tuple<>>;
template class NLogNBaseServer<SrcIDb1Tuple>;
//template class NLogNBaseServer<Tuple<IdAlias>;
