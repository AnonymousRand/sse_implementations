#include "schemes/pi_bas.h"

#include "schemes/pi_bas_server.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
PiBas<DbDoc, DbKw>::~PiBas() {
    this->clear();
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    this->prfKey = genKey(secParam);
    this->encKey = genKey(secParam);

    EncInd* encInd = new EncInd();
    encInd->init(this->size);

    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    Ind<DbKw, DbDoc> ind;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        DbDoc dbDoc = entry.first;
        Range<DbKw> dbKwRange = entry.second;
        if (ind.count(dbKwRange) == 0) {
            ind[dbKwRange] = std::vector {dbDoc};
        } else {
            ind[dbKwRange].push_back(dbDoc);
        }
    }
    // randomly permute documents associated with same keyword, required by some schemes on top of PiBas (e.g. Log-SRC)
    shuffleInd(ind);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        std::vector<DbDoc> dbKwList = iter->second;

        // for each id in DB(w)
        for (int64_t dbKwCounter = 0; dbKwCounter < dbKwList.size(); dbKwCounter++) {
            DbDoc dbDoc = dbKwList[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
            ustring label;
            uint64_t pos = this->map(queryToken, dbKwCounter, label);
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(Constants::IV_LEN);
            ustring encDbDoc = padAndEncrypt(
                Constants::ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncInd::DOC_LEN - 1
            );
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            encInd->write(pos, std::pair {label, std::pair {encDbDoc, iv}});
        }
    }

    this->server->setEncInd(encInd);
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBas<DbDoc, DbKw>::clear() {
    IStaticPointSse<DbDoc, DbKw>::clear();
    ISdaUnderly<DbDoc, DbKw>::clear();
    if (this->server != nullptr) {
        this->server->clear();
    }
}


//------------------------------------------------------------------------------
// `ISdaUnderly`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBas<DbDoc, DbKw>::getDb(Db<DbDoc, DbKw>& ret) const {
    EncInd* encInd = this->server->getEncInd();
    // don't use `this->size()` as the bound here as that doesn't include padding
    // while `encInd` does (this should all be client-side anyway so not leaking anything)
    for (int64_t pos = 0; pos < encInd->getSize(); pos++) {
        EncIndVal encIndVal;
        bool isValidVal = encInd->read(pos, encIndVal);
        if (!isValidVal) {
            continue;
        }

        DbDoc dbDoc = this->decryptEncIndVal(encIndVal);
        // this is where we use the fact that `DbDoc`s also store their `DbKw` ranges
        // to easily access these `DbKw` ranges in plaintext
        Range<DbKw> dbKwRange = dbDoc.getDbKwRange();
        ret.push_back(std::pair {dbDoc, dbKwRange});
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);
    std::vector<EncIndVal> encResults = this->server->searchEncInd(queryToken);
    for (EncIndVal encResult : encResults) {
        DbDoc result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ustring PiBas<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
uint64_t PiBas<DbDoc, DbKw>::map(const ustring& queryToken, int64_t dbKwCounter, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w) || c)
    retLabel = hash(Constants::HASH_FUNC, Constants::HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
    return hashToPos(retLabel);
}


template class PiBas<Doc<>, Kw>;               // default/standalone
template class PiBas<SrcIDb1Doc, Kw>;          // Log-SRC-i index 1
//template class PiBas<Doc<IdAlias>, IdAlias>; // Log-SRC-i index 2
