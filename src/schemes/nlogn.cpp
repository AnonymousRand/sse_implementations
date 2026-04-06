#include "nlogn.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
Nlogn<DbDoc, DbKw>::~Nlogn() {
    this->clear();
    if (this->dbKwListSizeDict != nullptr) {
        delete this->dbKwListSizeDict;
        this->dbKwListSizeDict = nullptr;
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Nlogn<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    long numLvls = std::ceil(std::log2(this->size)) + 1;
    for (long i = 0; i < numLvls; i++) {
        EncInd* lvl = new EncInd();
        lvl->init(this->size);
        this->encIndLvls.push_back(lvl);
    }
    this->dbKwListSizeDict->init(this->size);

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

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // pad keyword list to the next power of two
        std::vector<DbDoc> dbKwList = iter->second;
        long dbKwListSize = dbKwList.size();
        if (!std::has_single_bit(dbKwListSize)) {
            long amountToPad = std::pow(2, std::ceil(std::log2(dbKwListSize))) - dbKwListSize;
            dbKwList.reserve(dbKwListSize + amountToPad);
            for (long i = 0; i < amountToPad; i++) {
                // notice we even use dummy range for the db keyword (i.e. `Range<dbKw>`)
                // to differentiate from dummies originating upstream in Log-SRC-i* padding etc. (needed for `getDb()`)
                // (also since doing this doesn't affect the correctness of NlogN or the purpose of the dummies)
                DbDoc dummyDbDoc {DUMMY, DUMMY, Op::DUMMY, DUMMY_RANGE<DbKw>()};
                DbEntry<DbDoc, DbKw> dummyDbEntry = DbEntry {dummyDbDoc, dbKwRange};
                dbKwList.push_back(dummyDbEntry);
            }
        }
        //// randomly permute documents associated with same keyword, i.e. shuffle within bucket
        //std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        long dbKwListPaddedSize = dbKwList.size();
        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `lvl` and `pos`
        ustring label;
        std::pair<ulong, ulong> lvlAndPos = this->map(queryToken, dbKwListSize, label);
        ulong lvl = lvlAndPos.first;
        ulong pos = lvlAndPos.second;

        // add `(w, dbKwListSize)` (non-padded size) to dict to compute what level to search
        ustring ivDict = genIv(IV_LEN);
        ustring encDbKwListSize = padAndEncrypt(
            ENC_CIPHER, this->encKey, toUstr(dbKwListSize), ivDict, EncInd::DOC_LEN - 1
        );
        this->dbKwListSizeDict->write(pos, std::pair {label, std::pair {ivDict, encDbKwListSize}});

        // for each id in DB(w) (write into same bucket consecutively)
        for (long dbKwCounter = 0; dbKwCounter < dbKwListPaddedSize; dbKwCounter++) {
            DbDoc dbDoc = dbKwList[dbKwCounter];
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncInd::DOC_LEN - 1);
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            this->encIndLvls[lvl]->write(pos + dbKwCounter, std::pair {label, std::pair {iv, encDbDoc}});
        }
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Nlogn<DbDoc, DbKw>::clear() {
    IStaticPointSse::clear();
    ISdaUnderlySse::clear();

    for (EncInd* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            delete lvl;
            lvl = nullptr;
        }
    }
    this->encIndLvls.clear();

    if (this->dbKwListSizeDict != nullptr) {
        this->dbKwListSizeDict->clear();
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Nlogn<DbDoc, DbKw>::getDb(Db<DbDoc, DbKw>& ret) const {
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
        // also exclude dummies/padding (from NlogN, but not from upstream SSE using NlogN as underly, for example)
        if (dbKwRange.size() > 1 || dbKwRange == DUMMY_RANGE<DbKw>()) {
            continue;
        }
        ret.push_back(std::pair {dbDoc, dbKwRange});
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> Nlogn<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // first retrieve the number of results/`dbKwListSize` to know what level to search (and how many dummies there are)
    ustring labelDict;
    ulong posDict = this->mapForDict(queryToken, labelDict);
    EncIndVal encIndValDict;
    bool isFoundDict = this->dbKwListSizeDict->find(pos, labelDict, encIndValDict);
    if (!isFoundDict) {
        return std::vector<DbDoc> {};
    }
    ustring encDbKwListSize = encIndVal.first;
    ustring ivDict = encIndVal.second;
    ustring decDbKwListSize = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbKwListSize, ivDict);
    long dbKwListSize = fromUstr(decDbKwListSize);
    long dbKwListPaddedSize = std::pow(2, std::ceil(std::log2(dbKwListSize))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<ulong, ulong> lvlAndPos = this->map(queryToken, dbKwListSize, label);
    ulong lvl = lvlAndPos.first;
    ulong pos = lvlAndPos.second;
    // always return entire bucket (`dbKwListPaddedSize` instead of `dbKwListSize`) from server to hide result size
    for (long dbKwCounter = 0; dbKwCounter < dbKwListPaddedSize; dbKwCounter++) {
        EncIndVal encIndVal;
        bool isFound = this->encInd->find(pos + dbKwCounter, label, encIndVal);
        if (!isFound) {
            break;
        }

        DbDoc result = this->decryptEncIndVal(encIndVal);
        results.push_back(result);
    }

    return results;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ustring Nlogn<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::pair<ulong, ulong> Nlogn<DbDoc, DbKw>::map(const ustring& queryToken, long dbKwListSize, ustring& retLabel) const {
    ulong lvl = std::log2(dbKwListSize); // require `dbKwListSize` to already be padded (also bottom level is 0)
    // l <- Hash(PRF(K_1, w))
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken);
    ulong pos = hashToPos(retLabel)
    // round `pos` down to make sure it is aligned to the start of a bucket
    pos = std::floor(pos / dbKwListSize) * dbKwListSize;
    return std::pair {lvl, pos};
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ulong Nlogn<DbDoc, DbKw>::mapForDict(const ustring& queryToken, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken);
    return hashToPos(retLabel); // not rounded
}


template class Nlogn<Doc<>, Kw>;
template class Nlogn<SrcIDb1Doc, Kw>;
//template class Nlogn<Doc<IdAlias>, IdAlias>;
