#include "log_src_i_star.h"

#include "utils/cryptography.h"


//==============================================================================
// `LogSrcIStarUnderly`
//==============================================================================


namespace underly {


template <class DbDoc, class DbKw> requires IsValidDbParams<DbDoc, DbKw>
void LogSrcIStarUnderly<DbDoc, DbKw>::setup(int secParam, const Db<DbDoc, DbKw>& db) {
    this->clear();
    
    //--------------------------------------------------------------------------
    // init things

    this->secParam = secParam;
    this->size = db.size();
    Range<DbKw> dbKwBounds = findDbKwBounds(db);
    long leafCount = dbKwBounds.size();
    // the key to avoiding the blowup of using NlogN as a black box is by using `leafCount` instead of `db.size()` here,
    // since `db.size()` includes the replicated tuples and using it sort of assumes those are only the "raw" entries
    long numLvls = std::ceil(std::log2(leafCount)) + 1;
    for (long i = 0; i < numLvls; i++) {
        EncInd* lvl = new EncInd();
        long lvlSize;
        if (i == 0) {
            lvlSize = leafCount;
        } else {
            // this gives the TDAG node/bucket count at level `i` (for `i` >= 1)
            lvlSize = std::pow(2, numLvls - i) - 1;
            // then multiply by bucket size at level `i`
            lvlSize *= std::pow(2, i);
        }
        lvl->init(lvlSize);
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
    //// randomly permute documents associated with same keyword, i.e. shuffle within bucket
    //shuffleInd(ind);

    // for each w in W
    std::unordered_set<Range<DbKw>> uniqDbKwRanges = getUniqDbKwRanges(db);
    for (Range<DbKw> dbKwRange : uniqDbKwRanges) {
        auto iter = ind.find(dbKwRange);
        if (iter == ind.end()) {
            continue;
        }

        // generate a single `lvl`, `pos`, and `l` for each keyword list/bucket
        // note that there's no need to pad keyword lists to powers of two here
        // since that's guaranteed by upstream TDAG padding
        std::vector<DbDoc> dbKwList = iter->second;
        long dbKwListSize = dbKwList.size();
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
        this->dbKwListSizeDict->write(pos, std::pair {label, std::pair {encDbKwListSize, ivDict}});

        // for each id in DB(w) (write into same bucket consecutively)
        for (long dbKwCounter = 0; dbKwCounter < dbKwListSize; dbKwCounter++) {
            DbDoc dbDoc = dbKwList[dbKwCounter];
            // d <- Enc(K_2, w, id)
            ustring iv = genIv(IV_LEN);
            ustring encDbDoc = padAndEncrypt(ENC_CIPHER, this->encKey, dbDoc.toUstr(), iv, EncInd::DOC_LEN - 1);
            // store `(l, d)` into key-value store, and also store IV in plain along with `d`
            this->encIndLvls[lvl]->write(pos + dbKwCounter, std::pair {label, std::pair {encDbDoc, iv}});
        }
    }
}


template class LogSrcIStarUnderly<Doc<>, Kw>;       
template class LogSrcIStarUnderly<SrcIDb1Doc, Kw>;
//template class LogSrcIStarUnderly<Doc<IdAlias>, IdAlias>;


} // namespace `underly`


//==============================================================================
// `LogSrcIStar`
//==============================================================================


void LogSrcIStar::setup(int secParam, const Db<Doc<>, Kw>& db) {
    this->clear();

    this->secParam = secParam;
    this->size = db.size();
    this->origDbUnderly->setup(secParam, db);

    //--------------------------------------------------------------------------
    // build index 2

    // sort documents by keyword
    auto sortByKw = [](const DbEntry<Doc<>, Kw>& dbEntry1, const DbEntry<Doc<>, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };
    Db<Doc<>, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);

    // assign index 2 nodes/"identifier aliases" and populate both `db1` and `db2` leaves with this information
    Db<SrcIDb1Doc, Kw> db1;
    Db<Doc<IdAlias>, IdAlias> db2;
    long dbSortedSize = dbSorted.size();
    db1.reserve(dbSortedSize);
    db2.reserve(dbSortedSize);
    Kw prevKw = DUMMY;
    IdAlias firstIdAliasWithKw;
    IdAlias lastIdAliasWithKw;
    auto addDb1Leaf = [&db1](Kw prevKw, IdAlias firstIdAliasWithKw, IdAlias lastIdAliasWithKw) {
        Range<IdAlias> idAliasRangeWithKw {firstIdAliasWithKw, lastIdAliasWithKw};
        Range<Kw> kwRange {prevKw, prevKw};
        SrcIDb1Doc newDoc {prevKw, idAliasRangeWithKw, kwRange};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry {newDoc, kwRange};
        db1.push_back(newDbEntry);
    };
    for (long idAlias = 0; idAlias < dbSortedSize; idAlias++) {
        DbEntry<Doc<>, Kw> dbEntry = dbSorted[idAlias];
        Doc<> doc = dbEntry.first;
        Kw kw = dbEntry.second.first; // entries in `db` must have size 1 `Kw` ranges!
        // populate `db2` leaves
        Range<IdAlias> idAliasRange {idAlias, idAlias};
        Doc<IdAlias> newDb2Doc(doc.get(), idAliasRange);
        DbEntry<Doc<IdAlias>, IdAlias> newDb2Entry = DbEntry {newDb2Doc, idAliasRange};
        db2.push_back(newDb2Entry);

        // populate `db1` leaves
        if (kw != prevKw) {
            if (prevKw != DUMMY) {
                addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
            }
            prevKw = kw;
            firstIdAliasWithKw = idAlias;
            lastIdAliasWithKw = idAlias;
        } else {
            lastIdAliasWithKw = idAlias;
        }
    }
    // make sure to write in last `Kw` (which cannot be detected by `kw != prevKw` in the loop above)
    if (prevKw != DUMMY) {
        addDb1Leaf(prevKw, firstIdAliasWithKw, lastIdAliasWithKw);
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = 0;
    for (DbEntry<Doc<IdAlias>, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    // pad TDAG 2 leaf count to the next power of two, as is required for Log-SRC-i*
    long db2Size = db2.size();
    if (!std::has_single_bit((ulong)db2Size)) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db2Size))) - db2Size;
        db2.reserve(db2Size + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxIdAlias++;
            Range<IdAlias> idAliasRange {maxIdAlias, maxIdAlias};
            Doc<IdAlias> dummyDoc = Doc<IdAlias>::genDummy(idAliasRange);
            DbEntry<Doc<IdAlias>, IdAlias> dummyDbEntry = DbEntry {dummyDoc, idAliasRange};
            db2.push_back(dummyDbEntry);
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(Range<IdAlias> {0, maxIdAlias});

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    db2Size = db2.size();
    db2.reserve(calcTdagItemCount(db2Size));
    for (long i = 0; i < db2Size; i++) {
        DbEntry<Doc<IdAlias>, IdAlias> dbEntry = db2[i];
        Doc<IdAlias> doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            if (ancestor == idAliasRange) {
                continue;
            }
            Doc<IdAlias> newDoc(doc.get(), ancestor);
            db2.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly2->setup(secParam, db2);

    //--------------------------------------------------------------------------
    // build index 1

    // build TDAG 1 over `Kw`s
    // since `Kw`s have no guarantee of being contiguous but the leaves and hence bottom level in the index must be,
    // we need to pad `db1` to have (exactly) one doc per `Kw` (we can just leave blanks in the case of non-locality
    // Log-SRC-i since docs are placed pseudorandomly in the index, but here we have to pad to avoid empty buckets
    // in the index that the server knows corresponds to a lack of docs with that keyword)
    DbEntry<Doc<>, Kw> dbEntry = dbSorted[0];
    prevKw = dbEntry.second.first;
    for (long i = 1; i < dbSortedSize; i++) {
        dbEntry = dbSorted[i];
        Kw kw = dbEntry.second.first;
        // if non-contiguous `Kw`s detected, fill in the gap with dummies
        if (kw - prevKw > 1) {
            for (Kw paddingKw = prevKw + 1; paddingKw < kw; paddingKw++) {
                Range<Kw> paddingKwRange {paddingKw, paddingKw};
                SrcIDb1Doc dummyDoc = SrcIDb1Doc::genDummy(paddingKwRange);
                DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
                db1.push_back(dummyDbEntry);
            }
        }
        prevKw = kw;
    }
    // after guaranteeing contiguous-ness of `Kw`s, pad `db1` to power of 2 as well
    long db1Size = db1.size();
    Range<Kw> db1KwBounds = findDbKwBounds(db1);
    Kw maxDb1Kw = db1KwBounds.second;
    if (!std::has_single_bit((ulong)db1Size)) {
        long amountToPad = std::pow(2, std::ceil(std::log2(db1Size))) - db1Size;
        db1.reserve(db1Size + amountToPad);
        for (long i = 0; i < amountToPad; i++) {
            maxDb1Kw++;
            Range<Kw> paddingKwRange {maxDb1Kw, maxDb1Kw};
            SrcIDb1Doc dummyDoc = SrcIDb1Doc::genDummy(paddingKwRange);
            DbEntry<SrcIDb1Doc, Kw> dummyDbEntry = DbEntry {dummyDoc, paddingKwRange};
            db1.push_back(dummyDbEntry);
        }
    }
    this->tdag1 = new TdagNode<Kw>(Range {db1KwBounds.first, maxDb1Kw});

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    db1Size = db1.size();
    db1.reserve(calcTdagItemCount(db1Size));
    for (long i = 0; i < db1Size; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            SrcIDb1Doc newDoc(doc.get(), ancestor);
            db1.push_back(std::pair {newDoc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
}
