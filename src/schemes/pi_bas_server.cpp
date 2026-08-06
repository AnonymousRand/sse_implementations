#include "pi_bas_server.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
PiBasServer<DbDoc, DbKw>::PiBasServer(Benchmark& benchmark) : benchmark(benchmark) {}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
PiBasServer<DbDoc, DbKw>::~PiBasServer() {
    this->clear();
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    };
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::clear() {
    if (this->encInd != nullptr) {
        this->encInd->clear();
    };
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
void PiBasServer<DbDoc, DbKw>::setEncInd(EncInd* encInd) {
    this->benchmark.totalComm += encInd->getSize() * EncInd::ENTRY_LEN;
    this->encInd = encInd;
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
std::vector<EncIndVal> PiBasServer<DbDoc, DbKw>::search(const ustring& queryToken) const {
    this->benchmark.totalComm += queryToken.length();
    std::vector<EncIndVal> encResults;

    // for c = 0 until `Get` returns error
    long dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos` (same as client's `setup()`)
        ustring label = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
        ulong pos = hashToPos(label);
        // res <- encInd.get(l)
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        encResults.push_back(encIndVal);
        this->benchmark.totalComm += EncInd::VAL_LEN;
        dbKwCounter++;
    }

    return encResults;
}


template <class DbDoc, class DbKw> requires IsValidDbParams <DbDoc, DbKw>
bool PiBasServer<DbDoc, DbKw>::getEncIndVal(ulong pos, EncIndVal& ret) const {
    this->benchmark.totalComm += EncInd::VAL_LEN;
    return this->encInd->read(pos, ret);
};


template class PiBasServer<Doc<>, Kw>;
template class PiBasServer<SrcIDb1Doc, Kw>;
//template class PiBasServer<Doc<IdAlias>, IdAlias>;
