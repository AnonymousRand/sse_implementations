#include "schemes/pi_bas/pi_bas_server.h"

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h"
#include "utils/crypto.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


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
        this->benchmark->diskSize -= this->encInd->getSize() * EncInd::ENTRY_LEN;
        delete this->encInd;
        this->encInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
void PiBasServer<DbTuple>::setEncInd(EncInd* encInd) {
    int64_t encIndBytes = encInd->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->diskSize += encIndBytes;
    this->benchmark->network += encIndBytes;
    this->encInd = encInd;
}


template <IsDbTuple DbTuple>
EncInd* PiBasServer<DbTuple>::getEncInd() const {
    this->benchmark->network += this->encInd->getSize() * EncInd::ENTRY_LEN;
    return this->encInd;
}


template <IsDbTuple DbTuple>
std::vector<EncIndVal> PiBasServer<DbTuple>::searchEncInd(const ustring& queryToken) const {
    this->benchmark->network += queryToken.length();
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    int64_t dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
        // (same as client's `setup()`)
        ustring label = crypto::hash(
            crypto::HASH_FUNC, crypto::HASH_OUTPUT_LEN, queryToken + utils::toUstr(dbKwCounter)
        );
        uint64_t pos = utils::hashToPos(label);
        // res <- encInd.get(l)
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        dbKwCounter++;
    }

    this->benchmark->network += encResults.size() * EncInd::VAL_LEN;
    return encResults;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBasServer<Tuple<>>;
template class PiBasServer<SrcIDb1Tuple>;
//template class PiBasServer<Tuple<IdAlias>>;
