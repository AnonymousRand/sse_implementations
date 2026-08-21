#include "schemes/n_log_n/n_log_n.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/crypto.h"
#include "utils/misc.h"
#include "utils/random.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind.h"
#include "utils/types/ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


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
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    this->lvlCount = this->calcLvlCount();

    this->prfKey = utils::crypto::genKey(secParam);
    this->encKey = utils::crypto::genKey(secParam);
    
    std::vector<EncIndLoc*> encIndLvls;
    for (bigint lvl = 0; lvl < this->lvlCount; lvl++) {
        EncIndLoc* encIndLvl = new EncIndLoc(this->benchmark);
        bigint bcktCountOnLvl = this->calcBcktCountOnLvl(lvl);
        bigint bcktSizeOnLvl = this->calcBcktSizeOnLvl(lvl);
        encIndLvl->init(bcktSizeOnLvl, bcktCountOnLvl);
        encIndLvls.push_back(encIndLvl);
    }
    EncIndRand* dbKwCountsDict = new EncIndRand(this->benchmark);
    dbKwCountsDict->init(this->size);

    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping
    Ind<DbTuple> ind(db);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = db.getUniqDbKwRanges();
    for (const Range<DbKw>& dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            std::cerr << "Error: NLogN::setup(): DB kw range " << dbKwRange
                      << " not found in index" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // pad keyword list to the next power of two
        Db<DbTuple> dbKwList = std::move(iter->second);
        bigint dbKwCount = dbKwList.size();
        DbKw maxDbKw = db.getDbKwBounds().second;
        dbKwList.pad(maxDbKw);
        // randomly permute documents associated with same keyword, i.e. shuffle within bucket
        dbKwList.shuffle();

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        bigint dbKwPaddedCount = dbKwList.size();
        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        // l <- Hash(PRF(K_1, w) || c), and also generate associated `lvl` and `pos`
        ustring label;
        std::pair<ubigint, ubigint> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
        ubigint lvl = lvlAndPos.first;
        ubigint pos = lvlAndPos.second;

        // add `(w, dbKwCount)` (non-padded size) to dict to compute what level to search
        ustring labelDict;
        ustring ivDict = utils::crypto::genIv();
        ustring encDbKwCount = utils::crypto::padAndEncrypt(
            this->encKey, utils::ustr::toUstr(dbKwCount), ivDict, EncIndBase::DATA_LEN - 1
        );
        ubigint posDict = this->mapNoMod(queryToken, labelDict);
        dbKwCountsDict->writeToFirstEmpty(
            posDict, std::pair {labelDict, std::pair {encDbKwCount, ivDict}}
        );

        // for each id in DB(w) (write into same bucket consecutively)
        bigint bcktSizeOnLvl = this->calcBcktSizeOnLvl(lvl);
        ubigint startPos = pos * bcktSizeOnLvl;
        for (bigint dbKwCounter = 0; dbKwCounter < dbKwPaddedCount; dbKwCounter++) {
            DbTuple dbTuple = dbKwList[dbKwCounter];
            // d <- Enc(K_2, w, id)
            ustring iv = utils::crypto::genIv();
            ustring encDbTuple = utils::crypto::padAndEncrypt(
                this->encKey, dbTuple.toUstr(), iv, EncIndBase::DATA_LEN - 1
            );
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            if (dbKwCounter == 0) {
                // if first write to this bucket, get the first bucket start pos at or after
                // `startPos` that is *empty* (e.g. in case of modulo collision in encrypted index)
                encIndLvls[lvl]->writeToFirstEmpty(
                    startPos, std::pair {label, std::pair {encDbTuple, iv}}
                );
            } else {
                // after first write, just write consecutively as we are now guaranteed that
                // there is a full bucket of contiguous space here
                encIndLvls[lvl]->write(
                    startPos + dbKwCounter, std::pair {label, std::pair {encDbTuple, iv}}
                );
            }
        }
    }

    this->server->setEncIndLvls(encIndLvls);
    this->server->setDbKwCountsDict(dbKwCountsDict);
}


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::clear() {
    this->server->clear();
    this->lvlCount = 0;

    // clears `this->size`
    ISdUnderly<DbTuple>::clear();

    // clears keys
    IStaticPointSse<DbTuple>::clear();
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::getDb(Db<DbTuple>& ret) const {
    std::vector<EncIndLoc*> encIndLvls = this->server->getEncIndLvls();

    for (bigint lvl = 0; lvl < this->lvlCount; lvl++) {
        EncIndLoc* encIndLvl = encIndLvls[lvl];
        // don't use `this->size` as the bound here as that doesn't include padding while
        // `encIndLvl` does (this should all be client-side anyway so not leaking anything)
        for (bigint pos = 0; pos < encIndLvl->getSize(); pos++) {
            EncIndVal encIndVal;
            bool isValidVal = encIndLvl->read(pos, encIndVal);
            if (!isValidVal) {
                continue;
            }

            DbTuple dbTuple = this->decryptEncIndVal(encIndVal);
            // exclude dummies/padding (that are from NLogN's `setup()`, but not from any upstream
            // SSE scheme which is using NLogN as an underlying scheme. while deleting those dummies
            // too seems to work fine, we don't since we don't have an easy, general way to check
            // for those here, and that should be the upstream scheme's concern anyway.)
            if (!DbTuple::isDummy(dbTuple)) {
                // this is where we use the fact that `DbTuple`s also store their `DbKw` ranges
                // to easily access these `DbKw` ranges in plaintext
                DbTuple newDbTuple(dbTuple.getDbDoc(), dbTuple.getDbKwRange());
                ret.push_back(newDbTuple);
            }
        }
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <IsDbTuple DbTuple>
std::vector<DbTuple> NLogN<DbTuple>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbTuple> results {};

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);

    // first retrieve the number of results/`dbKwCount` to know what level to search
    // (and how many dummies there are)
    ustring labelDict;
    ubigint posDict = this->mapNoMod(queryToken, labelDict);
    EncIndVal encIndValDict;
    bool isFoundDict = this->server->getDbKwCount(posDict, labelDict, encIndValDict);
    if (!isFoundDict) {
        return results;
    }
    ustring encDbKwCount = encIndValDict.first;
    ustring ivDict = encIndValDict.second;
    ustring decDbKwCount = utils::crypto::decryptAndUnpad(this->encKey, encDbKwCount, ivDict);
    bigint dbKwCount = utils::ustr::fromUstr(decDbKwCount);
    bigint dbKwPaddedCount = std::pow(2, std::ceil(std::log2(dbKwCount))); // this is bucket size

    // compute `lvl` and `pos` of correct bucket (the same way as in `setup()`)
    ustring label;
    std::pair<ubigint, ubigint> lvlAndPos = this->map(queryToken, dbKwPaddedCount, label);
    ubigint lvl = lvlAndPos.first;
    ubigint pos = lvlAndPos.second;
    // return entire bucket (`dbKwPaddedCount` instead of `dbKwCount`) from server
    // to hide true result size
    ubigint startPos = pos * this->calcBcktSizeOnLvl(lvl);
    std::vector<EncIndVal> encResults = this->server->searchEncIndForBckt(
        lvl, startPos, dbKwPaddedCount, label
    );

    // decrypt results on the client
    results.reserve(encResults.size());
    for (const EncIndVal& encResult : encResults) {
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
    return utils::crypto::prf(this->prfKey, query.toUstr());
}


template <IsDbTuple DbTuple>
ubigint NLogN<DbTuple>::mapNoMod(const ustring& queryToken, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = utils::crypto::hash(queryToken);
    return utils::misc::hashToPos(retLabel); // no modulus
}


template <IsDbTuple DbTuple>
std::pair<ubigint, ubigint> NLogN<DbTuple>::map(
    const ustring& queryToken, bigint dbKwPaddedCount, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w))
    ubigint pos = this->mapNoMod(queryToken, retLabel);
    // (note bottommost level is level 0)
    ubigint lvl = std::log2(dbKwPaddedCount);
    pos %= (ubigint)this->calcBcktCountOnLvl(lvl);
    return std::pair {lvl, pos};
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::calcLvlCount() const {
    return std::ceil(std::log2(this->size)) + 1;
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::calcBcktCountOnLvl(bigint lvl) const {
    // 2^{lvlCount - lvl + 1} is number of buckets on level `lvl`
    return std::pow(2, this->lvlCount - lvl - 1);
}


template <IsDbTuple DbTuple>
bigint NLogN<DbTuple>::calcBcktSizeOnLvl(bigint lvl) const {
    return std::pow(2, lvl);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class NLogN<Tuple<>>;
template class NLogN<SrcIDb1Tuple>;
//template class NLogN<Tuple<IdAlias>>;
