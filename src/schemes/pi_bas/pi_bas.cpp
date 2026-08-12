#include "schemes/pi_bas/pi_bas.h"

#include <concepts>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#include "schemes/interfaces/sd_underly.h"
#include "schemes/interfaces/static_point_sse.h"
#include "schemes/pi_bas/pi_bas_server.h"

#include "utils/crypto.h"
#include "utils/db.h"
#include "utils/enc_ind.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
PiBas<DbRecord, DbKw>::~PiBas() {
    this->clear();
    if (this->server != nullptr) {
        delete this->server;
        this->server = nullptr;
    }
}


//------------------------------------------------------------------------------
// `ISse`


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void PiBas<DbRecord, DbKw>::setup(int secParam, const Db<DbRecord, DbKw>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();

    this->prfKey = utils::genKey(secParam);
    this->encKey = utils::genKey(secParam);

    EncInd* encInd = new EncInd();
    encInd->init(this->size);

    //--------------------------------------------------------------------------
    // build index

    // generate (plaintext) index of keywords to documents/ids mapping and list of unique keywords
    // and randomly permute documents associated with same keyword, required by
    // some schemes on top of PiBas (e.g. Log-SRC)
    Ind<DbKw, DbRecord> ind = utils::genInd(db, true);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = utils::getUniqDbKwRanges(db);
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        std::vector<DbRecord> dbKwList = iter->second;

        // for each id in DB(w)
        for (int64_t dbKwCounter = 0; dbKwCounter < dbKwList.size(); dbKwCounter++) {
            DbRecord dbRecord = dbKwList[dbKwCounter];
            // l <- Hash(PRF(K_1, w) || c), and also generate associated `pos`
            ustring label;
            uint64_t pos = this->map(queryToken, dbKwCounter, label);
            // d <- Enc(K_2, w, id)
            ustring iv = utils::genIv(utils::IV_LEN);
            ustring encDbRecord = utils::padAndEncrypt(
                utils::ENC_CIPHER, this->encKey, dbRecord.toUstr(), iv, EncInd::DOC_LEN - 1
            );
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            encInd->write(pos, std::pair {label, std::pair {encDbRecord, iv}});
        }
    }

    this->server->setEncInd(encInd);
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void PiBas<DbRecord, DbKw>::clear() {
    IStaticPointSse<DbRecord, DbKw>::clear();
    ISdUnderly<DbRecord, DbKw>::clear();

    this->server->clear();
}


//------------------------------------------------------------------------------
// `ISdUnderly`


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
void PiBas<DbRecord, DbKw>::getDb(Db<DbRecord, DbKw>& ret) const {
    EncInd* encInd = this->server->getEncInd();
    // don't use `this->size()` as the bound here as that doesn't include padding
    // while `encInd` does (this should all be client-side anyway so not leaking anything)
    for (int64_t pos = 0; pos < encInd->getSize(); pos++) {
        EncIndVal encIndVal;
        bool isValidVal = encInd->read(pos, encIndVal);
        if (!isValidVal) {
            continue;
        }

        DbRecord dbRecord = this->decryptEncIndVal(encIndVal);
        // this is where we use the fact that `DbRecord`s also store their `DbKw` ranges
        // to easily access these `DbKw` ranges in plaintext
        Range<DbKw> dbKwRange = dbRecord.getDbKwRange();
        ret.push_back(std::pair {dbRecord, dbKwRange});
    }
}


//------------------------------------------------------------------------------
// `IStaticPointSse`


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
std::vector<DbRecord> PiBas<DbRecord, DbKw>::searchBase(const Range<DbKw>& query) const {
    std::vector<DbRecord> results;

    // PRF(K_1, w)
    ustring queryToken = this->genQueryToken(query);
    std::vector<EncIndVal> encResults = this->server->searchEncInd(queryToken);
    for (EncIndVal encResult : encResults) {
        DbRecord result = this->decryptEncIndVal(encResult);
        results.push_back(result);
    }

    return results;
}


//------------------------------------------------------------------------------
// other


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
ustring PiBas<DbRecord, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return utils::prf(this->prfKey, query.toUstr());
}


template <class DbRecord, class DbKw> requires IsValidDbParams<DbRecord, DbKw>
uint64_t PiBas<DbRecord, DbKw>::map(
    const ustring& queryToken, int64_t dbKwCounter, ustring& retLabel
) const {
    // l <- Hash(PRF(K_1, w) || c)
    retLabel = utils::hash(
        utils::HASH_FUNC, utils::HASH_OUTPUT_LEN, queryToken + utils::toUstr(dbKwCounter)
    );
    return utils::hashToPos(retLabel);
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class PiBas<Record<>, Kw>;               // default/standalone
template class PiBas<SrcIDb1Record, Kw>;          // Log-SRC-i index 1
//template class PiBas<Record<IdAlias>, IdAlias>;   // Log-SRC-i index 2
