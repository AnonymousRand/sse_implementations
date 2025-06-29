#include "log_src_i.h"
#include "pi_bas.h"


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrcI<Underly>::LogSrcI() : underly1(new Underly<SrcIDb1Doc, Kw>()), underly2(new Underly<Doc, IdAlias>()) {}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrcI<Underly>::LogSrcI(EncIndType encIndType)
        : LogSrcI(new Underly<SrcIDb1Doc, Kw>(), new Underly<Doc, IdAlias>(), encIndType) {}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrcI<Underly>::LogSrcI(Underly<SrcIDb1Doc, Kw>* underly1, Underly<Doc, IdAlias>* underly2, EncIndType encIndType)
        : underly1(underly1), underly2(underly2) {
    this->setEncIndType(encIndType);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
LogSrcI<Underly>::~LogSrcI() {
    this->clear();
    if (this->underly1 != nullptr) {
        delete this->underly1;
        this->underly1 = nullptr;
    }
    if (this->underly2 != nullptr) {
        delete this->underly2;
        this->underly2 = nullptr;
    }
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrcI<Underly>::setup(int secParam, const Db<Doc, Kw>& db) {
    this->db = db;
    this->_isEmpty = this->db.empty();
    // so we don't leak the memory from the previous TDAGs after we call `new` again
    if (this->tdag1 != nullptr) {
        delete this->tdag1;
        this->tdag1 = nullptr;
    }
    if (this->tdag2 != nullptr) {
        delete this->tdag2;
        this->tdag2 = nullptr;
    }

    ////////////////////////////// build index 2 ///////////////////////////////

    // sort documents by keyword to assign index 2 nodes/"identifier aliases"
    auto sortByKw = [](const DbEntry<Doc, Kw>& dbEntry1, const DbEntry<Doc, Kw>& dbEntry2) {
        return dbEntry1.first.getKw() < dbEntry2.first.getKw();
    };

    Db<Doc, Kw> dbSorted = db;
    std::sort(dbSorted.begin(), dbSorted.end(), sortByKw);
    Db<Doc, IdAlias> db2;
    std::unordered_map<Id, IdAlias> idAliasMapping; // for quick reference when buiding index 1
    for (ulong i = 0; i < dbSorted.size(); i++) {
        DbEntry<Doc, Kw> dbEntry = dbSorted[i];
        Doc doc = dbEntry.first;
        DbEntry<Doc, IdAlias> newDbEntry = DbEntry {doc, Range<IdAlias> {i, i}};
        db2.push_back(newDbEntry);
        Id id = doc.getId();
        idAliasMapping[id] = IdAlias(i);
    }

    // build TDAG 2 over id aliases
    IdAlias maxIdAlias = IdAlias(0);
    for (DbEntry<Doc, IdAlias> dbEntry : db2) {
        IdAlias idAlias = dbEntry.second.first;
        if (idAlias > maxIdAlias) {
            maxIdAlias = idAlias;
        }
    }
    this->tdag2 = new TdagNode<IdAlias>(maxIdAlias);

    // replicate every document to all id alias ranges/TDAG 2 nodes that cover it
    ulong stop = db2.size();
    for (ulong i = 0; i < stop; i++) {
        DbEntry<Doc, IdAlias> dbEntry = db2[i];
        Doc doc = dbEntry.first;
        Range<IdAlias> idAliasRange = dbEntry.second;
        std::list<Range<IdAlias>> ancestors = this->tdag2->getLeafAncestors(idAliasRange);
        for (Range<IdAlias> ancestor : ancestors) {
            // ancestors include the leaf itself, which is already in `db2`
            if (ancestor == idAliasRange) {
                continue;
            }
            db2.push_back(std::pair {doc, ancestor});
        }
    }

    ////////////////////////////// build index 1 ///////////////////////////////

    // build TDAG 1 over keywords
    Kw maxKw = findMaxDbKw(db);
    this->tdag1 = new TdagNode<Kw>(maxKw);

    // assign id aliases/TDAG 2 nodes to documents based on index 2
    Db<SrcIDb1Doc, Kw> db1;
    for (DbEntry<Doc, Kw> dbEntry : db) {
        Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        IdAlias idAlias = idAliasMapping[doc.getId()];
        SrcIDb1Doc newDoc {kwRange, Range<IdAlias> {idAlias, idAlias}};
        DbEntry<SrcIDb1Doc, Kw> newDbEntry {newDoc, kwRange};
        db1.push_back(newDbEntry);
    }

    // replicate every document (in this case `SrcIDb1Doc`s) to all keyword ranges/TDAG 1 nodes that cover it
    stop = db1.size();
    for (ulong i = 0; i < stop; i++) {
        DbEntry<SrcIDb1Doc, Kw> dbEntry = db1[i];
        SrcIDb1Doc doc = dbEntry.first;
        Range<Kw> kwRange = dbEntry.second;
        std::list<Range<Kw>> ancestors = this->tdag1->getLeafAncestors(kwRange);
        for (Range<Kw> ancestor : ancestors) {
            if (ancestor == kwRange) {
                continue;
            }
            db1.push_back(std::pair {doc, ancestor});
        }
    }

    this->underly1->setup(secParam, db1);
    this->underly2->setup(secParam, db2);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
std::vector<Doc> LogSrcI<Underly>::search(const Range<Kw>& query, bool shouldProcessResults, bool isNaive) const {

    ///////////////////////////////// query 1 //////////////////////////////////

    Range<Kw> src1 = this->tdag1->findSrc(query);
    if (src1 == DUMMY_RANGE<Kw>()) { 
        return std::vector<Doc> {};
    }
    std::vector<SrcIDb1Doc> choices = this->underly1->search(src1, false, false);

    // generate query for query 2 based on query 1 results
    // (filter out unnecessary choices and merge remaining ones into a single id range)
    IdAlias minIdAlias = DUMMY;
    IdAlias maxIdAlias = DUMMY;
    for (SrcIDb1Doc choice : choices) {
        Range<Kw> choiceKwRange = choice.get().first;
        if (query.contains(choiceKwRange)) {
            Range<IdAlias> choiceIdAliasRange = choice.get().second;
            if (choiceIdAliasRange.first < minIdAlias || minIdAlias == DUMMY) {
                minIdAlias = choiceIdAliasRange.first;
            }
            if (choiceIdAliasRange.second > maxIdAlias || maxIdAlias == DUMMY) {
                maxIdAlias = choiceIdAliasRange.second;
            }
        }
    }

    ///////////////////////////////// query 2 //////////////////////////////////

    Range<IdAlias> query2 {minIdAlias, maxIdAlias};
    Range<IdAlias> src2 = this->tdag2->findSrc(query2);
    if (src2 == DUMMY_RANGE<IdAlias>()) {
        return std::vector<Doc> {};
    }
    return this->underly2->search(src2, shouldProcessResults, false);
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrcI<Underly>::clear() {
    if (this->tdag1 != nullptr) {
        delete this->tdag1;
        this->tdag1 = nullptr;
    }
    if (this->tdag2 != nullptr) {
        delete this->tdag2;
        this->tdag2 = nullptr;
    }
    this->underly1->clear();
    this->underly2->clear();
    this->db.clear();
    this->_isEmpty = true;
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
Db<Doc, Kw> LogSrcI<Underly>::getDb() const {
    return this->db;
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
bool LogSrcI<Underly>::isEmpty() const {
    return this->_isEmpty;
}


template <template <class ...> class Underly> requires ISse_<Underly<Doc, Kw>>
void LogSrcI<Underly>::setEncIndType(EncIndType encIndType) {
    this->underly1->setEncIndType(encIndType);
    this->underly2->setEncIndType(encIndType);
}


template class LogSrcI<PiBas>;
template class LogSrcI<PiBasResHiding>;
