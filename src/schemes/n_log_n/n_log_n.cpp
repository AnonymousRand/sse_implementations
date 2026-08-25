#include "schemes/n_log_n/n_log_n.h"

#include <cmath>
#include <concepts>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/n_log_n/n_log_n_base.h"
#include "schemes/n_log_n/n_log_n_server.h"

#include "utils/crypto.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind/enc_ind_rand.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple>
NLogN<DbTuple>::~NLogN() {
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
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
    bool isFoundDict = this->getServer()->getDbKwCount(posDict, labelDict, encIndValDict);
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
    std::vector<EncIndVal> encResults = this->getServer()->searchEncIndForBckt(
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
void NLogN<DbTuple>::initSetupState() {
    NLogNBase<DbTuple>::initSetupState();

    this->dbKwCountsDict = new EncIndRand(this->benchmark);
    this->dbKwCountsDict->init(this->size);
}


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::setupDbKwList(Db<DbTuple>&& dbKwList) {
    // add `(w, dbKwCount)` (non-padded size) to `dbKwCountsDict` to compute what level to search
    bigint dbKwCount = dbKwList.size();
    ustring queryToken = this->genQueryToken(dbKwRange);
    ustring label;
    ustring iv = utils::crypto::genIv();
    ustring encDbKwCount = utils::crypto::padAndEncrypt(
        this->encKey, utils::ustr::toUstr(dbKwCount), iv, EncIndBase::DATA_LEN - 1
    );
    ubigint pos = this->mapNoMod(queryToken, label);
    this->dbKwCountsDict->writeToFirstEmpty(pos, std::pair {label, std::pair {encDbKwCount, iv}});

    // do the rest from `NLogNBase` (we have to `std::move()` *after* we are done using `dbKwList`)
    NLogNBase<DbTuple>::setupDbKwList(std::move(dbKwList));
}


template <IsDbTuple DbTuple>
void NLogN<DbTuple>::moveSetupStateToServer() {
    NLogNBase<DbTuple>::moveSetupStateToServer();

    this->getServer()->setDbKwCountsDict(this->dbKwCountsDict);
    if (this->dbKwCountsDictTmp != nullptr) {
        delete this->dbKwCountsDictTmp;
        this->dbKwCountsDictTmp = nullptr;
    }
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
