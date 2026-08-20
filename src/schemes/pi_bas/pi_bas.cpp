#include "schemes/pi_bas/pi_bas.h"

#include <concepts>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "utils/crypto.h"
#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/enc_ind.h"
#include "utils/types/ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple>
PiBas<DbTuple>::~PiBas() {
    this->clear();
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::setup(int secParam, const Db<DbTuple>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    this->prfKey = utils::crypto::genKey(secParam);
    this->encKey = utils::crypto::genKey(secParam);

    EncInd* encInd = new EncInd();
    encInd->init(this->size);

    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping
    // also randomly permute documents associated with same keyword, required by
    // some schemes on top of PiBas (e.g. Log-SRC)
    Ind<DbTuple> ind(db, true);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = db.getUniqDbKwRanges();
    for (const Range<DbKw>& dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            std::cerr << "Error: PiBas::setup(): DB kw range " << dbKwRange
                      << " not found in index" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // PRF(K_1, w)
        this->benchmark->startProfile("crypto");
        ustring queryToken = this->genQueryToken(dbKwRange);
        this->benchmark->endProfile("crypto");
        Db<DbTuple> dbKwList = std::move(iter->second);

        // for each id in DB(w)
        for (bigint dbKwCounter = 0; dbKwCounter < dbKwList.size(); dbKwCounter++) {
            DbTuple dbTuple = dbKwList[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
            ustring label;
            ubigint pos = this->map(queryToken, dbKwCounter, label);
            // d <- Enc(K_2, w, id)
            ustring iv = utils::crypto::genIv();
            this->benchmark->startProfile("crypto");
            ustring encDbTuple = utils::crypto::padAndEncrypt(
                this->encKey, dbTuple.toUstr(), iv, EncInd::DATA_LEN - 1
            );
            this->benchmark->endProfile("crypto");
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            encInd->write(pos, std::pair {label, std::pair {encDbTuple, iv}});
        }
    }

    this->server->setEncInd(encInd);
}


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::clear() {
    this->server->clear();

    // clears `this->size`
    ISdUnderly<DbTuple>::clear();

    // clears keys
    IStaticPointSse<DbTuple>::clear();
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::getDb(Db<DbTuple>& ret) const {
    EncInd* encInd = this->server->getEncInd();

    // don't use `this->size` as the bound here as that doesn't include padding while
    // `encInd` does (this should all be client-side anyway so not leaking anything)
    for (bigint pos = 0; pos < encInd->getSize(); pos++) {
        EncIndVal encIndVal;
        bool isValidVal = encInd->read(pos, encIndVal);
        if (!isValidVal) {
            continue;
        }

        DbTuple dbTuple = this->decryptEncIndVal(encIndVal);
        // this is where we use the fact that `DbTuple`s also store their `DbKw` ranges
        // to easily access these `DbKw` ranges in plaintext
        Range<DbKw> dbKwRange = dbTuple.getDbKwRange();
        DbTuple newDbTuple(dbTuple.getDbDoc(), dbKwRange);
        ret.push_back(newDbTuple);
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <IsDbTuple DbTuple>
std::vector<DbTuple> PiBas<DbTuple>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbTuple> results;

    // PRF(K_1, w)
    this->benchmark->startProfile("crypto");
    ustring queryToken = this->genQueryToken(query);
    this->benchmark->endProfile("crypto");
    std::vector<EncIndVal> encResults = this->server->searchEncInd(queryToken);

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
ustring PiBas<DbTuple>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return utils::crypto::prf(this->prfKey, query.toUstr());
}


template <IsDbTuple DbTuple>
ubigint PiBas<DbTuple>::map(
    const ustring& queryToken, bigint dbKwCounter, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w) || c)
    this->benchmark->startProfile("crypto");
    retLabel = utils::crypto::hash(queryToken + utils::ustr::toUstr(dbKwCounter));
    this->benchmark->endProfile("crypto");
    return utils::misc::hashToPos(retLabel);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBas<Tuple<>>;
template class PiBas<SrcIDb1Tuple>;
//template class PiBas<Tuple<IdAlias>>;
