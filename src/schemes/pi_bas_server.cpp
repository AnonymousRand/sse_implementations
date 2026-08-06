#include "pi_bas_server.h"


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
PiBasServer<DbDoc, DbKw>::~PiBasServer() {
    this->clear();
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    };
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::clear() {
    if (this->encInd != nullptr) {
        this->encInd->clear();
    };
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::setEncInd(EncInd* encInd) {
    this->encInd = encInd;
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
std::vector<EncIndVal> search(const ustring& queryToken) const {
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    long dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos` (same as in `setup()`)
        ustring label;
        ulong pos = this->map(queryToken, dbKwCounter, label);
        // res <- encInd.get(l)
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        dbKwCounter++;
    }

    return encResults;
}


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams <DbDoc, DbKw>
EncInd* PiBasServer<DbDoc, DbKw>::getEncInd() const {
    return this->encInd;
};
