#include "schemes/n_log_n/n_log_n_base.h"

#include <cmath>
#include <concepts>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/n_log_n/n_log_n_base_server.h"

#include "utils/crypto.h"
#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind/enc_ind_loc.h"
#include "utils/types/enc_ind/enc_ind_types.h"
#include "utils/types/ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::setup(int secParam, const Db<DbTuple>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    this->lvlCount = this->calcLvlCount();

    this->prfKey = utils::crypto::genKey(secParam);
    this->encKey = utils::crypto::genKey(secParam);

    this->initSetupState();
    
    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping
    Ind<DbTuple> ind(db);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = db.getUniqDbKwRanges();
    for (const Range<DbKw>& dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            std::cerr << "Error: NLogNBase::setup(): DB kw range " << dbKwRange
                      << " not found in index" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        this->setupDbKwList(std::move(iter->second), dbKwRange);
    }

    this->moveSetupStateToServer();
}


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::clear() {
    // IMPORTANT: `this->getServer()->clear()` cannot be done here as `getServer()`
    // is a pure virtual method, and `NLogNBase::clear()` is called in destructor
    // so, child classes having a concrete server member variable MUST handle that instead!
    this->lvlCount = 0;

    // clears `this->size`
    ISdUnderly<DbTuple>::clear();

    // clears keys
    IStaticPointSse<DbTuple>::clear();
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::getDb(Db<DbTuple>& ret) const {
    std::vector<EncIndLoc*> encIndLvls = this->getServer()->getEncIndLvls();

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
// helpers


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::initSetupState() {
    for (bigint lvl = 0; lvl < this->lvlCount; lvl++) {
        EncIndLoc* encIndLvl = new EncIndLoc(this->benchmark);
        bigint bcktCountOnLvl = this->calcBcktCountOnLvl(lvl);
        bigint bcktSizeOnLvl = this->calcBcktSizeOnLvl(lvl);
        encIndLvl->init(bcktSizeOnLvl, bcktCountOnLvl);
        this->encIndLvlsTmp.push_back(encIndLvl);
    }
}


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::setupDbKwList(Db<DbTuple>&& dbKwList, const Range<DbKw>& dbKwRange) {
    // pad keyword list to the next power of two
    dbKwList.padToPowOf2();
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
            this->encIndLvlsTmp[lvl]->writeToFirstEmpty(
                startPos, std::pair {label, std::pair {encDbTuple, iv}}
            );
        } else {
            // after first write, just write consecutively as we are now guaranteed that
            // there is a full bucket of contiguous space here
            this->encIndLvlsTmp[lvl]->write(
                startPos + dbKwCounter, std::pair {label, std::pair {encDbTuple, iv}}
            );
        }
    }
}


template <IsDbTuple DbTuple>
void NLogNBase<DbTuple>::moveSetupStateToServer() {
    // IMPORTANT: since this is a transfer of pointers, clearing it should be handled by the server!
    this->getServer()->setEncIndLvls(this->encIndLvlsTmp);
    // however we still need to clear the vector in the client so it doesn't
    // keep trying to call old instances later!!
    this->encIndLvlsTmp.clear();
}


template <IsDbTuple DbTuple>
ustring NLogNBase<DbTuple>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return utils::crypto::prf(this->prfKey, query.toUstr());
}


template <IsDbTuple DbTuple>
ubigint NLogNBase<DbTuple>::mapNoMod(const ustring& queryToken, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = utils::crypto::hash(queryToken);
    return utils::misc::hashToPos(retLabel); // no modulus
}


template <IsDbTuple DbTuple>
std::pair<ubigint, ubigint> NLogNBase<DbTuple>::map(
    const ustring& queryToken, bigint dbKwPaddedCount, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w))
    ubigint pos = this->mapNoMod(queryToken, retLabel);
    // (note bottommost level is level 0)
    ubigint lvl = std::log2(dbKwPaddedCount);
    pos %= (ubigint)this->calcBcktCountOnLvl(lvl);
    return std::pair {lvl, pos};
}


template class NLogNBase<Tuple<>>;
template class NLogNBase<SrcIDb1Tuple>;
//template class NLogNBase<Tuple<IdAlias>>;
