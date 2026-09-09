#include "schemes/n_log_n/n_log_n_server.h"

#include <concepts>

#include "schemes/n_log_n/n_log_n_base_server.h"

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// `ISseServer`


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::clear() {
    if (this->dbKwCountsDict != nullptr) {
        utils::benchmark::serverStorage -= this->dbKwCountsDict->getBytes();

        delete this->dbKwCountsDict;
        this->dbKwCountsDict = nullptr;
    }

    NLogNBaseServer<DbTuple>::clear();
}


//------------------------------------------------------------------------------
// interface


template <IsDbTuple DbTuple>
void NLogNServer<DbTuple>::setDbKwCountsDict(EncIndRand* dbKwCountsDict) {
    bigint dbKwCountsDictBytes = dbKwCountsDict->getBytes();
    utils::benchmark::serverStorage += dbKwCountsDictBytes;
    utils::benchmark::communication += dbKwCountsDictBytes;
    this->dbKwCountsDict = dbKwCountsDict;
}


template <IsDbTuple DbTuple>
bool NLogNServer<DbTuple>::getDbKwCount(ubigint pos, const ustring& label, EncIndVal& ret) const {
    utils::benchmark::communication +=
        sizeof(ubigint) + label.length() + this->dbKwCountsDict->VAL_LEN();
    return this->dbKwCountsDict->find(pos, label, ret);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogNServer<Tuple<>>;
template class NLogNServer<SrcIDb1Tuple>;
//template class NLogNServer<Tuple<IdAlias>;
