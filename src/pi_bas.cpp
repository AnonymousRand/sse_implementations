#include "pi_bas.h"

#include "utils/cryptography.h"


//==============================================================================
// `PiBasBase`
//==============================================================================


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> PiBasBase<DbDoc, DbKw>::search(
    const Range<DbKw>& query, bool shouldCleanUpResults, bool isNaive
) const {
    std::vector<DbDoc> allResults;

    if (isNaive) {
        // naive range search: just individually query every point in range
        for (DbKw dbKw = query.first; dbKw <= query.second; dbKw++) {
            std::vector<DbDoc> results = this->searchBase(Range {dbKw, dbKw});
            allResults.insert(allResults.end(), results.begin(), results.end());
        }
    } else {
        // search entire range in one go (i.e. `query` itself must be in the db), e.g. as underlying for Log-SRC
        allResults = this->searchBase(query);
    }

    if (shouldCleanUpResults) {
        cleanUpResults(allResults);
    }
    return allResults;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ustring PiBasBase<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasBase<DbDoc, DbKw>::clear() {
    this->size = 0;
    this->prfKey = toUstr("");
    this->encKey = toUstr("");
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBasBase<DbDoc, DbKw>::getDb(Db<DbDoc, DbKw>& ret) const {
    std::pair<ustring, ustring> encIndVal;
    for (long i = 0; i < this->size; i++) {
        bool isValidVal = this->readEncIndValFromPos(i, encIndVal);
        if (!isValidVal) {
            continue;
        }
        ustring encDbDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        ustring decDbDoc = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbDoc, iv);
        DbDoc dbDoc = DbDoc::fromUstr(decDbDoc);
        // this is where we use the fact that `DbDoc`s also store their `DbKw` ranges
        // to easily access these `DbKw` ranges in plaintext
        Range<DbKw> dbKwRange = dbDoc.getDbKwRange();
        // de-replicate tuples: assume any tuples with `DbKw` range size >1 is non-leaf and hence a replicated tuple
        if (dbKwRange.size() > 1) {
            continue;
        }
        ret.push_back(std::pair {dbDoc, dbKwRange});
    }
}


template class PiBasBase<Doc<>, Kw>;               // PiBas
template class PiBasBase<SrcIDb1Doc, Kw>;          // Log-SRC-i index 1
//template class PiBasBase<Doc<IdAlias>, IdAlias>; // Log-SRC-i index 2


//==============================================================================
// `PiBas`
//==============================================================================


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
PiBas<DbDoc, DbKw>::~PiBas() {
    this->clear();
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBas<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();

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
    // randomly permute documents associated with same keyword, required by some schemes on top of PiBas (e.g. Log-SRC)
    shuffleInd(ind);

    this->encInd->init(db.size());
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        // this is PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);

        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }
        std::vector<DbDoc> dbDocsWithSameDbKw = iter->second;
        // for each id in DB(w)
        for (long dbKwCounter = 0; dbKwCounter < dbDocsWithSameDbKw.size(); dbKwCounter++) {
            DbDoc dbDoc = dbDocsWithSameDbKw[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c)
            ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            // for some reason padding to exactly n blocks generates n + 1 blocks, so we pad to one less byte
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncIndBase::DOC_LEN - 1);
            // store (l, d) into key-value store
            // also store IV in plain along with encrypted value
            this->encInd->write(label, std::pair {encDbDoc, iv});
        }
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> PiBas<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;
    ustring queryToken = this->genQueryToken(query);

    // for c = 0 until `Get` returns error
    long dbKwCounter = 0;
    while (true) {
        // l <- Hash(PRF(K_1, w) || c) (same as in `setup()`!)
        ustring label = findHash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken + toUstr(dbKwCounter));
        // res <- encInd.get(l)
        std::pair<ustring, ustring> encIndVal;
        bool isFound = this->encInd->find(label, encIndVal);
        if (!isFound) {
            break;
        }
        ustring encDbDoc = encIndVal.first;
        ustring iv = encIndVal.second;
        // technically we decrypt in the client, but since there's no client-server distinction in this implementation
        // we'll just decrypt immediately to make the code cleaner
        ustring decDbDoc = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbDoc, iv);
        DbDoc result = DbDoc::fromUstr(decDbDoc);
        results.push_back(result);
        dbKwCounter++;
    }

    return results;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void PiBas<DbDoc, DbKw>::clear() {
    PiBasBase<DbDoc, DbKw>::clear();
    if (this->encInd != nullptr) {
        this->encInd->clear();
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
bool PiBas<DbDoc, DbKw>::readEncIndValFromPos(ulong pos, std::pair<ustring, ustring>& ret) const {
    return this->encInd->readValFromPos(pos, ret);
}


template class PiBas<Doc<>, Kw>;
template class PiBas<SrcIDb1Doc, Kw>;
//template class PiBas<Doc<IdAlias>, IdAlias>;
