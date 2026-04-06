#include "pibas.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
Pibas<DbDoc, DbKw>::~Pibas() {
    this->clear();
    if (this->encInd != nullptr) {
        delete this->encInd;
        this->encInd = nullptr;
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Pibas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();
    this->encInd->init(this->size);

    //--------------------------------------------------------------------------
    // generate keys

    this->prfKey = genKey(secParam);
    this->encKey = genKey(secParam);

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
    // randomly permute documents associated with same keyword, required by some schemes on top of Pibas (e.g. Log-SRC)
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
        for (long dbKwCounter = 0; dbKwCounter < dbKwList.size(); dbKwCounter++) {
            DbDoc dbDoc = dbKwList[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c) and also generate associated `pos`
            ustring label;
            ulong pos = this->map(queryToken, dbKwCounter, label);
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncInd::DOC_LEN - 1);
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            this->encInd->write(pos, std::pair {label, std::pair {iv, encDbDoc}});
        }
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Pibas<DbDoc, DbKw>::clear() {
    this->size = 0;
    this->prfKey = toUstr("");
    this->encKey = toUstr("");
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Pibas<DbDoc, DbKw>::getDb(Db<DbDoc, DbKw>& ret) const {
    for (long i = 0; i < this->size; i++) {
        EncIndVal encIndVal;
        bool isValidVal = this->encInd->read(i, encIndVal);
        if (!isValidVal) {
            continue;
        }

        DbDoc dbDoc = this->decryptEncIndVal(encIndVal);
        // this is where we use the fact that `DbDoc`s also store their `DbKw` ranges
        // to easily access these `DbKw` ranges in plaintext
        Range<DbKw> dbKwRange = dbDoc.getDbKwRange();
        // exclude replicated tuples: assume any tuples with `DbKw` range size >1 is non-leaf and hence replicated
        if (dbKwRange.size() > 1) {
            continue;
        }
        ret.push_back(std::pair {dbDoc, dbKwRange});
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> Pibas<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);
    // for c = 0 until `Get` returns error
    long dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c) and also generate associated `pos` (same as in `setup()`!)
        ustring label;
        ulong pos = this->map(queryToken, dbKwCounter, label);
        // res <- encInd.get(l)
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos, label, encIndVal);
        if (!isFound) {
            break;
        }

        DbDoc result = this->decryptEncIndVal(encIndVal);
        results.push_back(result);
        dbKwCounter++;
    }

    return results;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ustring Pibas<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ulong Pibas<DbDoc, DbKw>::map(const ustring& queryToken, long dbKwCounter, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w) || c)
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
    return hashToPos(retLabel);
}


template class Pibas<Doc<>, Kw>;               // default/standalone
template class Pibas<SrcIDb1Doc, Kw>;          // Log-SRC-i index 1
//template class Pibas<Doc<IdAlias>, IdAlias>; // Log-SRC-i index 2
