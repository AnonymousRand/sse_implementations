#include "schemes/pi_bas/pi_bas_server.h"

#include <concepts>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind.h"
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
    // (this is deleted instead of just cleared since we only set it via direct
    // pointer assignment, so if we don't delete we would make this memory inaccessible
    // the next time we assign `encInd`)
    if (this->encInd != nullptr) {
        this->benchmark->serverStorage -= this->encInd->getSize() * EncInd::ENTRY_LEN;
        delete this->encInd;
        this->encInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void PiBasServer<DbTuple>::setEncInd(EncInd* encInd) {
    bigint encIndBytes = encInd->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->serverStorage += encIndBytes;
    this->benchmark->communication += encIndBytes;
    this->encInd = encInd;
}


template <IsDbTuple DbTuple>
EncInd* PiBasServer<DbTuple>::getEncInd() const {
    this->benchmark->communication += this->encInd->getSize() * EncInd::ENTRY_LEN;
    return this->encInd;
}


template <IsDbTuple DbTuple>
std::vector<EncIndVal> PiBasServer<DbTuple>::searchEncInd(const ustring& queryToken) const {
    this->benchmark->communication += queryToken.length();
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    bigint dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
        // (same as client's `setup()`)
        ustring label = utils::crypto::hash(queryToken + utils::ustr::toUstr(dbKwCounter));
        ubigint pos = utils::misc::hashToPos(label);
        // res <- encInd.get(l)
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        dbKwCounter++;
    }

    this->benchmark->communication += encResults.size() * EncInd::VAL_LEN;
    return encResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBasServer<Tuple<>>;
template class PiBasServer<SrcIDb1Tuple>;
//template class PiBasServer<Tuple<IdAlias>>;
