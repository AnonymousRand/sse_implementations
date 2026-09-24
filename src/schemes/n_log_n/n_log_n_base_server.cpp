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


bigint calcAllEncrIndLvlsBytes(const std::vector<EncrIndLoc*>& encIndLvls) {
    bigint bytes = 0;
    for (EncrIndLoc* encIndLvl : encIndLvls) {
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
    for (EncrIndLoc* lvl : this->encIndLvls) {
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
void NLogNBaseServer<DbTuple>::setEncrIndLvls(const std::vector<EncrIndLoc*>& encIndLvls) {
    bigint allEncrIndLvlsBytes = ::calcAllEncrIndLvlsBytes(encIndLvls);
    utils::benchmark::serverStorage += allEncrIndLvlsBytes;
    utils::benchmark::communication += allEncrIndLvlsBytes;

    this->encIndLvls = encIndLvls;
}


template <IsDbTuple DbTuple>
const std::vector<EncrIndLoc*>& NLogNBaseServer<DbTuple>::getEncrIndLvls() const {
    utils::benchmark::communication += ::calcAllEncrIndLvlsBytes(this->encIndLvls);

    return this->encIndLvls;
}


template <IsDbTuple DbTuple>
std::vector<EncrIndVal> NLogNBaseServer<DbTuple>::searchEncrIndForBckt(
    bigint lvl, ubigint startPos, bigint bcktSize, const ustring& label
) const {
    assert(lvl < this->encIndLvls.size() || this->encIndLvls.size() == 0);
    utils::benchmark::communication +=
        sizeof(bigint) + sizeof(ubigint) + sizeof(bigint) + label.length();

    std::vector<EncrIndVal> encResults;
    for (bigint dbKwCounter = 0; dbKwCounter < bcktSize; dbKwCounter++) {
        EncrIndVal encIndVal;
        bool isFound;
        if (dbKwCounter == 0) {
            // if first read, get the right bucket start pos (e.g. in case of modulo
            // collision in encrypted index)
            // (NOTE: dummies must also use the correct (not dummy) `label` so they
            // are still found by `find()`!)
            isFound = this->encIndLvls[lvl]->find(
                SseOper::SEARCH, startPos, label, encIndVal
            );
        } else {
            // after first read, just read from the bucket consecutively as we are
            // now guaranteed that the full bucket is stored here contiguously
            // 
            // we also stop `fseek()`ing at every read since the read itself should advance
            // the file pointer to the right location for the next one
            isFound = this->encIndLvls[lvl]->read(
                SseOper::SEARCH, startPos + dbKwCounter, encIndVal, false
            );
        }
        if (!isFound) {
            break;
        }

        encResults.emplace_back(std::move(encIndVal));
        utils::benchmark::communication += this->encIndLvls[lvl]->VAL_LEN();
    }

    return encResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNBaseServer<Tuple<>>;
template class NLogNBaseServer<SrcIDb1Tuple>;
//template class NLogNBaseServer<Tuple<IdAlias>;
