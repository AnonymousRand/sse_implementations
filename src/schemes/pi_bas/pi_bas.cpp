#include "schemes/pi_bas/pi_bas.h"

#include <cassert>
#include <concepts>
#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/interfaces/i_sd_underly.h"
#include "schemes/interfaces/i_static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "utils/crypto.h"
#include "utils/debug.h"
#include "utils/str.h"
#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/encr_ind/encr_ind_rand.h"
#include "utils/types/encr_ind/encr_ind_types.h"
#include "utils/types/ind.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"
#include "utils/types/ustring.h"


template <IsDbTuple DbTuple>
PiBas<DbTuple>::~PiBas() {
    this->clear();

    if (this->piBasServer != nullptr) {
        delete this->piBasServer;
        this->piBasServer = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::setup(int secParam, const Db<DbTuple>& db, SseOper setupOper) {
    assert(this->piBasServer != nullptr);
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.getSize();

    this->prfKey = utils::crypto::genKey(secParam);
    this->encKey = utils::crypto::genKey(secParam);

    EncrIndRand* encInd = new EncrIndRand();
    encInd->init(setupOper, this->size);

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
        DEBUG_ONLY({
            if (iter == ind.end()) {
                std::cerr << "Error: PiBas::setup(): DB kw range " << dbKwRange
                          << " not found in index" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        });

        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        Db<DbTuple> dbKwList = std::move(iter->second);

        // for each id in DB(w)
        for (bigint dbKwCounter = 0; dbKwCounter < dbKwList.getSize(); dbKwCounter++) {
            DbTuple dbTuple = dbKwList[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
            ustring label;
            ubigint pos = this->map(queryToken, dbKwCounter, label);
            // d <- Enc(K_2, w, id)
            ustring iv = utils::crypto::genIv();
            ustring encDbTuple = utils::crypto::padAndEncrypt(
                this->encKey, dbTuple.encode(), iv, encInd->DATA_LEN() - 1
            );
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            encInd->writeToFirstEmpty(
                setupOper, pos, EncrIndEntry {label, EncrIndVal {encDbTuple, iv}}
            );
        }
    }

    encInd->endSetup(setupOper);
    this->piBasServer->setEncrInd(encInd);
}


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::clear() {
    assert(this->piBasServer != nullptr);
    this->piBasServer->clear();

    // clears `this->size`
    ISdUnderly<DbTuple>::clear();

    // clears keys
    IStaticPointSse<DbTuple>::clear();
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <IsDbTuple DbTuple>
void PiBas<DbTuple>::getDb(Db<DbTuple>& ret) const {
    assert(this->piBasServer != nullptr);
    EncrIndRand* encInd = this->piBasServer->getEncrInd();

    // don't use `this->size` as the bound here as that doesn't include padding while
    // `encInd` does (this should all be client-side anyway so it's not leaking anything)
    for (bigint pos = 0; pos < encInd->getCapacity(); pos++) {
        EncrIndVal encIndVal;
        bool isValidVal = encInd->read(SseOper::UPDATE, pos, encIndVal);
        if (!isValidVal) {
            continue;
        }

        DbTuple dbTuple = this->decryptEncrIndVal(encIndVal);
        ret.append(dbTuple);
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <IsDbTuple DbTuple>
std::vector<typename PiBas<DbTuple>::DbDoc> PiBas<DbTuple>::searchRaw(
    const Range<DbKw>& query
) const {
    assert(this->piBasServer != nullptr);
    std::vector<DbDoc> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);
    std::vector<EncrIndVal> encResultTups = this->piBasServer->searchEncrInd(queryToken);

    // decrypt results (on the client)
    results.reserve(encResultTups.size());
    for (const EncrIndVal& encResultTup : encResultTups) {
        DbTuple resultTup = this->decryptEncrIndVal(encResultTup);
        results.emplace_back(std::move(resultTup.dbDoc));
    }

    return results;
}


//------------------------------------------------------------------------------
// helpers


template <IsDbTuple DbTuple>
ustring PiBas<DbTuple>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return utils::crypto::prf(this->prfKey, query.encode());
}


template <IsDbTuple DbTuple>
ubigint PiBas<DbTuple>::map(
    const ustring& queryToken, bigint dbKwCounter, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w) || c)
    retLabel = utils::crypto::hash(queryToken + utils::str::encodeBigint(dbKwCounter));
    return utils::str::hashToPos(retLabel);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBas<Tuple<>>;
template class PiBas<SrcIDb1Tuple>;
//template class PiBas<Tuple<IdAlias>>;
