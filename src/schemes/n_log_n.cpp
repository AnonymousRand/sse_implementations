#include "schemes/n_log_n.h"

#include <cmath>

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
NLogN<DbDoc, DbKw>::~NLogN() {
    this->clear();
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogN<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    this->numLvls = this->computeNumLvls();

    this->prfKey = genKey(secParam);
    this->encKey = genKey(secParam);
    
    std::vector<EncInd*> encIndLvls;
    for (int64_t lvlNum = 0; lvlNum < this->numLvls; lvlNum++) {
        EncInd* lvl = new EncInd();
        int64_t bcktCountOnLvl = this->computeBcktCountOnLvl(lvlNum);
        int64_t bcktSizeOnLvl = this->computeBcktSizeOnLvl(lvlNum);
        lvl->init(bcktCountOnLvl * bcktSizeOnLvl);
        encIndLvls.push_back(lvl);
    }
    // TODO << overload for db entries??
    EncInd* dbKwCountsDict = new EncInd();
    dbKwCountsDict->init(this->size);
    //std::cerr << "--------- Setup beginning, db is:" << std::endl;
    //for (auto entry : db) {
    //    std::cerr << entry.first << std::endl;
    //}
    //std::cerr << std::endl;

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
        int64_t dbKwCount = dbKwList.size();
        if (!std::has_single_bit((uint64_t)dbKwCount)) {
            int64_t amountToPad = std::pow(2, std::ceil(std::log2(dbKwCount))) - dbKwCount;
            dbKwList.reserve(dbKwCount + amountToPad);
            // notice we even use dummy range for the db keyword (i.e. `Range<DbKw>`)
            // to differentiate from dummies originating upstream in Log-SRC-i* padding etc. (needed for `getDb()`)
            // (also since doing this doesn't affect the correctness of NLogN or the purpose of the dummies)
            DbDoc dummyDbDoc = DbDoc::genDummy(DUMMY_RANGE<DbKw>());
            for (int64_t i = 0; i < amountToPad; i++) {
                dbKwList.push_back(dummyDbDoc);
            }
        }
        //// randomly permute documents associated with same keyword, i.e. shuffle within bucket
        //std::shuffle(dbKwList.begin(), dbKwList.end(), RNG);

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        int64_t dbKwPaddedCount = dbKwList.size();
        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `lvl` and `pos`
        ustring label;
        std::pair<uint64_t, uint64_t> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
        uint64_t lvl = lvlAndPos.first;
        uint64_t pos = lvlAndPos.second;

        // add `(w, dbKwCount)` (non-padded size) to dict to compute what level to search
        ustring labelDict;
        ustring ivDict = genIv(IV_LEN);
        ustring encDbKwCount = padAndEncrypt(
            ENC_CIPHER, this->encKey, toUstr(dbKwCount), ivDict, EncInd::DOC_LEN - 1
        );
        uint64_t posDict = this->mapNoMod(queryToken, labelDict);
        dbKwCountsDict->write(posDict, std::pair {labelDict, std::pair {encDbKwCount, ivDict}});

        // for each id in DB(w) (write into same bucket consecutively)
        uint64_t startPos = pos * this->computeBcktSizeOnLvl(lvl);
        //std::cerr << "Writing " << dbKwRange << " with unpadded count " << dbKwCount << " and padded count " << dbKwPaddedCount << " returned lvl " << lvl << " pos " << pos << " with startPos " << startPos << " (bucket size " << this->computeBcktSizeOnLvl(lvl) << ")" << std::endl;
        //std::cerr << "-- Writing kw list:" << std::endl;
        //for (auto entry : dbKwList) {
        //    std::cerr << entry << std::endl;
        //}
        for (int64_t dbKwCounter = 0; dbKwCounter < dbKwPaddedCount; dbKwCounter++) {
            DbDoc dbDoc = dbKwList[dbKwCounter];
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncInd::DOC_LEN - 1);
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            std::cerr << "writing " << dbDoc << "," << dbKwRange << ", trying lvl " << lvl << " pos " << startPos + dbKwCounter << "; total size is " << this->size << std::endl;
            encIndLvls[lvl]->write(startPos + dbKwCounter, std::pair {label, std::pair {encDbDoc, iv}});
        }
    }
    std::cerr << std::endl;

    this->server->setEncIndLvls(encIndLvls);
    this->server->setDbKwCountsDict(dbKwCountsDict);
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogN<DbDoc, DbKw>::clear() {
    IStaticPointSse<DbDoc, DbKw>::clear();
    ISdaUnderly<DbDoc, DbKw>::clear();

    if (this->server != nullptr) {
        this->server->clear();
    }
    this->numLvls = 0;
}


//------------------------------------------------------------------------------
// `ISdaUnderly`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void NLogN<DbDoc, DbKw>::getDb(Db<DbDoc, DbKw>& ret) const {
    std::vector<EncInd*> encIndLvls = this->server->getEncIndLvls();

    for (int64_t lvl = 0; lvl < this->numLvls; lvl++) {
        EncInd* encIndLvl = encIndLvls[lvl];
        // don't use `this->size()` as the bound here as that doesn't include padding
        // while `encIndLvl` does (this should all be client-side anyway so not leaking anything)
        std::cerr << "getting db: lvl " << lvl << ", claimed size is " << encIndLvl->getSize() << std::endl;
        for (int64_t pos = 0; pos < encIndLvl->getSize(); pos++) {
            EncIndVal encIndVal;
            bool isValidVal = encIndLvl->read(pos, encIndVal);
            if (!isValidVal) {
                continue;
            }

            DbDoc dbDoc = this->decryptEncIndVal(encIndVal);
            // this is where we use the fact that `DbDoc`s also store their `DbKw` ranges
            // to easily access these `DbKw` ranges in plaintext
            Range<DbKw> dbKwRange = dbDoc.getDbKwRange();
            // exclude replicated tuples: assume any tuples with `DbKw` range size >1 is non-leaf and hence replicated
            // also exclude dummies/padding (from NLogN, but not from upstream SSE using NLogN as underly, for example)
            if (dbKwRange.size() > 1 || dbKwRange == DUMMY_RANGE<DbKw>()) {
                continue;
            }
            ret.push_back(std::pair {dbDoc, dbKwRange});
        }
    }
    std::cerr << "finished getting db :3" << std::endl << std::endl;
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::vector<DbDoc> NLogN<DbDoc, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // first retrieve the number of results/`dbKwCount` to know what level to search (and how many dummies there are)
    ustring labelDict;
    uint64_t posDict = this->mapNoMod(queryToken, labelDict);
    EncIndVal encIndValDict;
    bool isFoundDict = this->server->getDbKwCount(posDict, labelDict, encIndValDict);
    if (!isFoundDict) {
        return std::vector<DbDoc> {};
    }
    ustring encDbKwCount = encIndValDict.first;
    ustring ivDict = encIndValDict.second;
    ustring decDbKwCount = decryptAndUnpad(ENC_CIPHER, this->encKey, encDbKwCount, ivDict);
    int64_t dbKwCount = fromUstr(decDbKwCount);
    int64_t dbKwPaddedCount = std::pow(2, std::ceil(std::log2(dbKwCount))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<uint64_t, uint64_t> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
    uint64_t lvl = lvlAndPos.first;
    uint64_t pos = lvlAndPos.second;
    // return entire bucket (`dbKwPaddedCount` instead of `dbKwCount`) from server to hide true result size
    uint64_t startPos = pos * this->computeBcktSizeOnLvl(lvl);
    std::cerr << "Search for " << query << " returned unpadded size of " << dbKwCount << " and padded size " << dbKwPaddedCount << "; on lvl " << lvl << " pos " << pos << " with startPos " << startPos << " (bucket size " << this->computeBcktSizeOnLvl(lvl) << ")" << std::endl;
    std::vector<EncIndVal> encResults = this->server->findEncIndBckt(lvl, startPos, dbKwPaddedCount, label);
    for (EncIndVal encResult : encResults) {
        DbDoc result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }
    std::cerr << "after decryption there are " << results.size() << " results";
    for (DbDoc result : results) {
        std::cout << ", " << result;
    }
    std::cout << std::endl << std::endl;

    return results;
}


//------------------------------------------------------------------------------
// other


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
ustring NLogN<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
uint64_t NLogN<DbDoc, DbKw>::mapNoMod(const ustring& queryToken, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken);
    return hashToPos(retLabel); // no modulus
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::pair<uint64_t, uint64_t> NLogN<DbDoc, DbKw>::map(
    const ustring& queryToken, int64_t dbKwPaddedCount, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w))
    uint64_t pos = this->mapNoMod(queryToken, retLabel);
    // (note bottommost level is level 0)
    uint64_t lvl = std::log2(dbKwPaddedCount);
    //std::cerr << "mapping with padded count " << dbKwPaddedCount << " returned lvl " << lvl << " pos " << pos << "; num buckets on lvl is " << (uint64_t)this->computeBcktCountOnLvl(lvl) << " so final pos is " << pos % (uint64_t)this->computeBcktCountOnLvl(lvl) << "; bucket size is " << this->computeBcktSizeOnLvl(lvl) << " and num lvls is " << this->numLvls << std::endl;
    pos %= (uint64_t)this->computeBcktCountOnLvl(lvl);
    return std::pair {lvl, pos};
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
int64_t NLogN<DbDoc, DbKw>::computeNumLvls() const {
    return std::ceil(std::log2(this->size)) + 1;
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
int64_t NLogN<DbDoc, DbKw>::computeBcktCountOnLvl(int64_t lvl) const {
    // 2^{lvlCount - lvl + 1} is number of buckets on level `lvl`
    return std::pow(2, this->numLvls - lvl - 1);
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
int64_t NLogN<DbDoc, DbKw>::computeBcktSizeOnLvl(int64_t lvl) const {
    return std::pow(2, lvl);
}


template class NLogN<Doc<>, Kw>;
template class NLogN<SrcIDb1Doc, Kw>;
//template class NLogN<Doc<IdAlias>, IdAlias>;
