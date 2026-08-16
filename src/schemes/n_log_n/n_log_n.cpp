#include "schemes/n_log_n/n_log_n.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/crypto.h"
#include "utils/db/db.h"
#include "utils/enc_ind.h"
#include "utils/ind.h"
#include "utils/misc.h"
#include "utils/random.h"
#include "utils/range.h"
#include "utils/tuple.h"
#include "utils/types.h"
#include "utils/ustring.h"


template <IsDbTuple DbTuple>
NLogN<DbTuple>::~NLogN() {
    this->clear();
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::setup(int secParam, const Db<DbTuple>& db) {
    std::cerr << "----- nlogn setup" << std::endl;
    this->clear();
    std::cerr << "----- nlogn cleared" << std::endl;
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    this->numLvls = this->computeNumLvls();

    this->prfKey = crypto::genKey(secParam);
    this->encKey = crypto::genKey(secParam);
    
    std::vector<EncInd*> encIndLvls;
    for (bigint lvlNum = 0; lvlNum < this->numLvls; lvlNum++) {
        EncInd* lvl = new EncInd();
        bigint bcktCountOnLvl = this->computeBcktCountOnLvl(lvlNum);
        bigint bcktSizeOnLvl = this->computeBcktSizeOnLvl(lvlNum);
        lvl->init(bcktCountOnLvl * bcktSizeOnLvl);
        encIndLvls.push_back(lvl);
    }
    EncInd* dbKwCountsDict = new EncInd();
    dbKwCountsDict->init(this->size);
    std::cerr << "----- nlogn init stuff done" << std::endl;

    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    Ind<DbTuple> ind(db);
    std::cerr << "----- nlogn index built" << std::endl;

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = db.getUniqDbKwRanges();
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // pad keyword list to the next power of two
        Db<DbTuple> dbKwList = iter->second;
        bigint dbKwCount = dbKwList.size();
        std::cerr << "----- nlogn keyword list with size " << dbKwCount << " and contents: " << std::endl;
        for (auto tuple : dbKwList) {
            std::cerr << tuple << std::endl;
        }
        Range<DbKw> dbKwBounds = db.findDbKwBounds();
        DbKw maxDbKw = dbKwBounds.second;
        dbKwList.pad(maxDbKw);
        std::cerr << "----- nlogn keyword list padded" << std::endl;
        // randomly permute documents associated with same keyword, i.e. shuffle within bucket
        dbKwList.shuffle();
        std::cerr << "----- nlogn keyword list shuffled" << std::endl;

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        bigint dbKwPaddedCount = dbKwList.size();
        std::cerr << "----- nlogn keyword list size is " << dbKwList.size() << " and contents are:" << std::endl;
        for (auto tuple : dbKwList) {
            std::cerr << tuple << std::endl;
        }
        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `lvl` and `pos`
        ustring label;
        std::pair<ubigint, ubigint> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
        ubigint lvl = lvlAndPos.first;
        ubigint pos = lvlAndPos.second;

        // add `(w, dbKwCount)` (non-padded size) to dict to compute what level to search
        ustring labelDict;
        ustring ivDict = crypto::genIv(crypto::IV_LEN);
        ustring encDbKwCount = crypto::padAndEncrypt(
            crypto::ENC_CIPHER, this->encKey, utils::toUstr(dbKwCount), ivDict, EncInd::DATA_LEN - 1
        );
        ubigint posDict = this->mapNoMod(queryToken, labelDict);
        dbKwCountsDict->write(posDict, std::pair {labelDict, std::pair {encDbKwCount, ivDict}});

        // for each id in DB(w) (write into same bucket consecutively)
        ubigint startPos = pos * this->computeBcktSizeOnLvl(lvl);
        for (bigint dbKwCounter = 0; dbKwCounter < dbKwPaddedCount; dbKwCounter++) {
            DbTuple dbTuple = dbKwList[dbKwCounter];
            // d <- Enc(K_2, w, id)
            ustring iv = crypto::genIv(crypto::IV_LEN);
            ustring encDbTuple = crypto::padAndEncrypt(
                crypto::ENC_CIPHER, this->encKey, dbTuple.toUstr(), iv, EncInd::DATA_LEN - 1
            );
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            encIndLvls[lvl]->write(
                startPos + dbKwCounter, std::pair {label, std::pair {encDbTuple, iv}}
            );
        }
        std::cerr << "----- nlogn keyword list done" << std::endl;
    }
    std::cerr << "----- nlogn all keyword lists done" << std::endl;

    this->server->setEncIndLvls(encIndLvls);
    this->server->setDbKwCountsDict(dbKwCountsDict);
    std::cerr << "----- nlogn setup done!!" << std::endl;
}


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::clear() {
    IStaticPointSse<DbTuple>::clear();
    ISdUnderly<DbTuple>::clear();

    this->server->clear();
    this->numLvls = 0;
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::getDb(Db<DbTuple>& ret) const {
    std::vector<EncInd*> encIndLvls = this->server->getEncIndLvls();

    for (bigint lvl = 0; lvl < this->numLvls; lvl++) {
        EncInd* encIndLvl = encIndLvls[lvl];
        // don't use `this->size()` as the bound here as that doesn't include padding
        // while `encIndLvl` does (this should all be client-side anyway so not leaking anything)
        for (bigint pos = 0; pos < encIndLvl->getSize(); pos++) {
            EncIndVal encIndVal;
            bool isValidVal = encIndLvl->read(pos, encIndVal);
            if (!isValidVal) {
                continue;
            }

            DbTuple dbTuple = this->decryptEncIndVal(encIndVal);
            // this is where we use the fact that `DbTuple`s also store their `DbKw` ranges
            // to easily access these `DbKw` ranges in plaintext
            Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
            // exclude dummies/padding (that are from NLogN's `setup()`, but not from any upstream
            // SSE scheme which is using NLogN as an underlying scheme. while deleting those dummies
            // too seems to work fine, we don't since we don't have an easy, general way to check
            // for those here, and that should be the upstream scheme's concern anyway.)
            if (dbKwRange != DUMMY_RANGE<DbKw>()) {
                DbTuple newDbTuple(dbTuple.getDbDoc(), dbKwRange);
                ret.push_back(newDbTuple);
            }
        }
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <IsDbTuple DbTuple>
std::vector<DbTuple> NLogN<DbTuple>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbTuple> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // first retrieve the number of results/`dbKwCount` to know what level to search
    // (and how many dummies there are)
    ustring labelDict;
    ubigint posDict = this->mapNoMod(queryToken, labelDict);
    EncIndVal encIndValDict;
    bool isFoundDict = this->server->getDbKwCount(posDict, labelDict, encIndValDict);
    if (!isFoundDict) {
        return std::vector<DbTuple> {};
    }
    ustring encDbKwCount = encIndValDict.first;
    ustring ivDict = encIndValDict.second;
    ustring decDbKwCount = crypto::decryptAndUnpad(
        crypto::ENC_CIPHER, this->encKey, encDbKwCount, ivDict
    );
    bigint dbKwCount = utils::fromUstr(decDbKwCount);
    bigint dbKwPaddedCount = std::pow(2, std::ceil(std::log2(dbKwCount))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<ubigint, ubigint> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
    ubigint lvl = lvlAndPos.first;
    ubigint pos = lvlAndPos.second;
    // return entire bucket (`dbKwPaddedCount` instead of `dbKwCount`) from server
    // to hide true result size
    ubigint startPos = pos * this->computeBcktSizeOnLvl(lvl);
    std::vector<EncIndVal> encResults = this->server->searchEncIndForBckt(
        lvl, startPos, dbKwPaddedCount, label
    );
    for (EncIndVal encResult : encResults) {
        DbTuple result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
ustring NLogN<DbTuple>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return crypto::prf(this->prfKey, query.toUstr());
}


template <IsDbTuple DbTuple>
ubigint NLogN<DbTuple>::mapNoMod(const ustring& queryToken, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = crypto::hash(crypto::HASH_FUNC, crypto::HASH_OUTPUT_LEN, queryToken);
    return utils::hashToPos(retLabel); // no modulus
}


template <IsDbTuple DbTuple>
std::pair<ubigint, ubigint> NLogN<DbTuple>::map(
    const ustring& queryToken, bigint dbKwPaddedCount, ustring& retLabel
) const {
    std::cerr << "----- nlogn map() called with dbKwPaddedCount " << dbKwPaddedCount << std::endl;
    // l <- Hash(PRF(K_1, w))
    ubigint pos = this->mapNoMod(queryToken, retLabel);
    // (note bottommost level is level 0)
    ubigint lvl = std::log2(dbKwPaddedCount);
    pos %= (ubigint)this->computeBcktCountOnLvl(lvl);
    return std::pair {lvl, pos};
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::computeNumLvls() const {
    return std::ceil(std::log2(this->size)) + 1;
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::computeBcktCountOnLvl(bigint lvl) const {
    // 2^{lvlCount - lvl + 1} is number of buckets on level `lvl`
    return std::pow(2, this->numLvls - lvl - 1);
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::computeBcktSizeOnLvl(bigint lvl) const {
    return std::pow(2, lvl);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogN<Tuple<>>;
template class NLogN<SrcIDb1Tuple>;
//template class NLogN<Tuple<IdAlias>>;
