#include "nlogn.h"

#include "utils/cryptography.h"


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
Nlogn<DbDoc, DbKw>::~Nlogn() {
    this->clear();
    for (EncInd* lvl : this->encIndLvls) {
        if (lvl != nullptr) {
            delete lvl;
            lvl = nullptr;
        }
    }
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void Nlogn<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();

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

    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    // for each w in W
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // PRF(K_1, w)
        ustring queryToken = this->genQueryToken(dbKwRange);
        std::vector<DbDoc> dbKwList = iter->second;

        // pad keyword list to the next power of two
        long dbKwListSize = dbKwList.size();
        if (!std::has_single_bit(dbKwListSize)) {
            long amountToPad = std::pow(2, std::ceil(std::log2(dbKwListSize))) - dbKwListSize;
            dbKwList.reserve(dbKwListSize + amountToPad);
            for (long i = 0; i < amountToPad; i++) {
                DbDoc dummyDbDoc {DUMMY, DUMMY, Op::DUMMY, dbKwRange};
                DbEntry<DbDoc, DbKw> dummyDbEntry = DbEntry {dummyDbDoc, dbKwRange};
                dbKwList.push_back(dummyDbEntry);
            }
        }

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        long dbKwListPaddedSize = dbKwList.size();
        // l <- Hash(PRF(K_1, w) || c) and also generate associated `lvl` and `pos`
        ustring label;
        std::pair<ulong, ulong> lvlAndPos = this->map(queryToken, dbKwListSize, label);
        ulong lvl = lvlAndPos.first;
        ulong pos = lvlAndPos.second;

        // add `(w, dbKwListSize)` (non-padded size) to dict to know how many non-dummies
        // there are in this block (corresponding to `w`) during search
        // (client will just decrypt first `dbKwListSize` entries in the block, so dummies must come after non-dummies)
        ustring iv = genIv(IV_LEN);
        ustring encDbKwListSize = padAndEncrypt(
            ENC_CIPHER, this->encKey, toUstr(dbKwListSize), iv, EncInd::DOC_LEN - 1
        );
        this->dbKwListSizeDict->write(pos, std::pair {label, std::pair {iv, encDbKwListSize}});

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
ustring Nlogn<DbDoc, DbKw>::genQueryToken(const Range<DbKw>& query) const {
    // PRF(K_1, w)
    return prf(this->prfKey, query.toUstr());
}


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
std::pair<ulong, ulong> Nlogn<DbDoc, DbKw>::map(const ustring& queryToken, long dbKwListSize, ustring& retLabel) const {
    // l <- Hash(PRF(K_1, w))
    retLabel = hash(HASH_FUNC, HASH_OUTPUT_LEN, queryToken);
    ulong lvl = std::log2(dbKwListSize); // require `dbKwListSize` to already be padded (also bottom level is 0)
    ulong pos = hashToPos(retLabel);
    return std::pair {lvl, pos};
}
