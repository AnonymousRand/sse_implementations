#include "schemes/pi_bas_server.h"

#include <concepts>
#include <cstdint>
#include <vector>

#include "schemes/interfaces/sse_server.h"

#include "utils/benchmark.h" // since this was only forward declared in `sse_server.h`
#include "utils/cryptography.h"
#include "utils/doc.h"
#include "utils/enc_ind.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
PiBasServer<DbDoc, DbKw>::~PiBasServer() {
    this->clear();
}


//------------------------------------------------------------------------------
// `ISseServer`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::clear() {
    // (this is deleted instead of just cleared since we only set it via direct pointer assignment,
    // so if we don't delete we would make this memory inaccessible the next time we assign `encInd`)
    if (this->encInd != nullptr) {
        this->benchmark->diskSize -= this->encInd->getSize() * EncInd::ENTRY_LEN;
        delete this->encInd;
        this->encInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::setEncInd(EncInd* encInd) {
    int64_t encIndBytes = encInd->getSize() * EncInd::ENTRY_LEN;
    this->benchmark->diskSize += encIndBytes;
    this->benchmark->network += encIndBytes;
    this->encInd = encInd;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
EncInd* PiBasServer<DbDoc, DbKw>::getEncInd() const {
    this->benchmark->network += this->encInd->getSize() * EncInd::ENTRY_LEN;
    return this->encInd;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<EncIndVal> PiBasServer<DbDoc, DbKw>::searchEncInd(const ustring& queryToken) const {
    this->benchmark->network += queryToken.length();
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    int64_t dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos` (same as client's `setup()`)
        ustring label = hash(constants::HASH_FUNC, constants::HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
        uint64_t pos = hashToPos(label);
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


template class PiBasServer<Doc<>, Kw>;
template class PiBasServer<SrcIDb1Doc, Kw>;
//template class PiBasServer<Doc<IdAlias>, IdAlias>;
