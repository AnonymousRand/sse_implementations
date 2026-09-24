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
    // this is deleted instead of cleared since we only set it via direct pointer assignment, so
    // if we don't delete we would make this memory inaccessible the next time we assign `encrInd`
    if (this->encrInd != nullptr) {
        utils::benchmark::serverStorage -= this->encrInd->getBytes();

        delete this->encrInd;
        this->encrInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void PiBasServer<DbTuple>::setEncrInd(EncrIndRand* encrInd) {
    bigint encrIndBytes = encrInd->getBytes();
    utils::benchmark::serverStorage += encrIndBytes;
    utils::benchmark::communication += encrIndBytes;
    this->encrInd = encrInd;
}


template <IsDbTuple DbTuple>
EncrIndRand* PiBasServer<DbTuple>::getEncrInd() const {
    utils::benchmark::communication += this->encrInd->getBytes();
    return this->encrInd;
}


template <IsDbTuple DbTuple>
std::vector<EncrIndVal> PiBasServer<DbTuple>::searchEncrInd(const ustring& queryToken) const {
    utils::benchmark::communication += queryToken.length();
    std::vector<EncrIndVal> encrResults;

    // for c = 0 until `Get` returns error
    bigint dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
        // (same as client's `setup()`)
        ustring label = utils::crypto::hash(queryToken + utils::str::encodeBigint(dbKwCounter));
        ubigint pos = utils::str::hashToPos(label);
        // res <- encrInd.get(l)
        EncrIndVal encrIndVal;
        bool isFound = this->encrInd->find(SseOper::SEARCH, pos, label, encrIndVal);
        if (!isFound) {
            break;
        }

        encrResults.emplace_back(std::move(encrIndVal));
        utils::benchmark::communication += this->encrInd->VAL_LEN();
        dbKwCounter++;
    }

    return encrResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBasServer<Tuple<>>;
template class PiBasServer<SrcIDb1Tuple>;
//template class PiBasServer<Tuple<IdAlias>>;
