#include "schemes/pi_bas/pi_bas_server.h"

#include <concepts>
#include <utility>
#include <vector>

#include "schemes/interfaces/i_sse_server.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/str.h"
#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_rand.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple>
PiBasServer<DbTuple>::~PiBasServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <IsDbTuple DbTuple>
void PiBasServer<DbTuple>::clear() {
    // this is deleted instead of just cleared since we only set it via direct pointer assignment,
    // so if we don't delete we would make this memory inaccessible the next time we assign `encInd`
    if (this->encInd != nullptr) {
        utils::benchmark::serverStorage -= this->encInd->getBytes();

        delete this->encInd;
        this->encInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void PiBasServer<DbTuple>::setEncrInd(EncrIndRand* encInd) {
    bigint encIndBytes = encInd->getBytes();
    utils::benchmark::serverStorage += encIndBytes;
    utils::benchmark::communication += encIndBytes;
    this->encInd = encInd;
}


template <IsDbTuple DbTuple>
EncrIndRand* PiBasServer<DbTuple>::getEncrInd() const {
    utils::benchmark::communication += this->encInd->getBytes();
    return this->encInd;
}


template <IsDbTuple DbTuple>
std::vector<EncrIndVal> PiBasServer<DbTuple>::searchEncrInd(const ustring& queryToken) const {
    utils::benchmark::communication += queryToken.length();
    std::vector<EncrIndVal> encResults;

    // for c = 0 until `Get` returns error
    bigint dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
        // (same as client's `setup()`)
        ustring label = utils::crypto::hash(queryToken + utils::str::encodeBigint(dbKwCounter));
        ubigint pos = utils::str::hashToPos(label);
        // res <- encInd.get(l)
        EncrIndVal encIndVal;
        bool isFound = this->encInd->find(SseOper::SEARCH, pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        encResults.emplace_back(std::move(encIndVal));
        utils::benchmark::communication += this->encInd->VAL_LEN();
        dbKwCounter++;
    }

    return encResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBasServer<Tuple<>>;
template class PiBasServer<SrcIDb1Tuple>;
//template class PiBasServer<Tuple<IdAlias>>;
