#include "schemes/pi_bas_server.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
PiBasServer<DbDoc, DbKw>::~PiBasServer() {
    this->clear();
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    };
}


//------------------------------------------------------------------------------
// `ISseServer`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::clear() {
    if (this->encInd != nullptr) {
        this->encInd->clear();
    };
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::setEncInd(EncInd* encInd) {
    this->benchmark->communication += encInd->getSize() * EncInd::ENTRY_LEN;
    this->encInd = encInd;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
EncInd* PiBasServer<DbDoc, DbKw>::getEncInd() const {
    this->benchmark->communication += this->encInd->getSize() * EncInd::ENTRY_LEN;
    return this->encInd;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<EncIndVal> PiBasServer<DbDoc, DbKw>::searchEncInd(const ustring& queryToken) const {
    this->benchmark->communication += queryToken.length();
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    int64_t dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos` (same as client's `setup()`)
        ustring label = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
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

    this->benchmark->communication += encResults.size() * EncInd::VAL_LEN;
    return encResults;
}


template class PiBasServer<Doc<>, Kw>;
template class PiBasServer<SrcIDb1Doc, Kw>;
//template class PiBasServer<Doc<IdAlias>, IdAlias>;
