#include "utils.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>


/******************************************************************************/
/* `ustring`                                                                  */
/******************************************************************************/


ustring toUstr(long n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}


ustring toUstr(const std::string& s) {
    return reinterpret_cast<const uchar*>(s.c_str());
}


ustring toUstr(uchar* ucstr, int len) {
    return ustring(ucstr, len);
}


std::string fromUstr(const ustring& ustr) {
    std::string str;
    for (uchar c : ustr) {
        str += static_cast<char>(c);
    }
    return str;
}


std::ostream& operator <<(std::ostream& os, const ustring& ustr) {
    return os << fromUstr(ustr);
}


/******************************************************************************/
/* `Range`                                                                    */
/******************************************************************************/


template <class T>
const std::regex Range<T>::FROM_STR_REGEX("(-?[0-9]+)-(-?[0-9]+)");


template <class T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T> {start, end} {}


template <class T>
T Range<T>::size() const {
    return this->second - this->first + 1;
}


template <class T>
bool Range<T>::contains(const Range<T>& target) const {
    return this->first <= target.first && this->second >= target.second;
}


template <class T>
bool Range<T>::contains(T target) const {
    return this->first <= target && this->second >= target;
}


template <class T>
bool Range<T>::isDisjointFrom(const Range<T>& target) const {
    return this->second < target.first || this->first > target.second;
}


template <class T>
std::string Range<T>::toStr() const {
    std::stringstream ss;
    ss << this->first << "-" << this->second;
    return ss.str();
}


template <class T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, Range<T>::FROM_STR_REGEX) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `Range.fromStr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }

    Range<T> range;
    range.first = T(std::stoi(matches[1].str()));
    range.second = T(std::stoi(matches[2].str()));
    return range;
}


template <class T>
ustring Range<T>::toUstr() const {
    return ::toUstr(this->toStr());
}


template <class T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toStr();
}


template class Range<Kw>;
// commented out since currently `IdAlias` and `Kw` are the same type (`long`)
//template class Range<IdAlias>;

template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
//template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);


/******************************************************************************/
/* `IDbDoc`                                                                   */
/******************************************************************************/


template <class T>
IDbDoc<T>::IDbDoc(const T& val) {
    this->val = val;
}


template <class T>
T IDbDoc<T>::get() const {
    return this->val;
}


template <class T>
ustring IDbDoc<T>::toUstr() const {
    return ::toUstr(this->toStr());
}


template <class T>
std::ostream& operator <<(std::ostream& os, const IDbDoc<T>& iDbDoc) {
    return os << iDbDoc.toStr();
}


/******************************************************************************/
/* `Doc`                                                                      */
/******************************************************************************/


const std::regex Doc::FROM_STR_REGEX("\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\)");


Doc::Doc(Id id, Kw kw, Op op) : IDbDoc<std::tuple<Id, Kw, Op>>(std::tuple {id, kw, op}) {}


std::string Doc::toStr() const {
    std::stringstream ss;
    ss << "(" << std::get<0>(this->val) << "," << std::get<1>(this->val) << ","
       << static_cast<char>(std::get<2>(this->val)) << ")";
    return ss.str();
}


Doc Doc::fromUstr(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    return Doc::fromStr(str);
}


Doc Doc::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, Doc::FROM_STR_REGEX) || matches.size() != 4) {
        std::cerr << "Error: bad string passed to `Doc.fromStr()`, the world is going to end now" << std::endl;
        exit(EXIT_FAILURE);
    }
    Id id = std::stoi(matches[1].str());
    Kw kw = std::stoi(matches[2].str());
    Op op = static_cast<Op>(matches[3].str()[0]);
    return Doc {id, kw, op};
}


Id Doc::getId() const {
    return std::get<0>(this->val);
}


Kw Doc::getKw() const {
    return std::get<1>(this->val);
}


Op Doc::getOp() const {
    return std::get<2>(this->val);
}


bool operator ==(const Doc& doc1, const Doc& doc2) {
    if (std::get<0>(doc1.val) != std::get<0>(doc2.val)) {
        return false;
    }
    if (std::get<1>(doc1.val) != std::get<1>(doc2.val)) {
        return false;
    }
    if (std::get<2>(doc1.val) != std::get<2>(doc2.val)) {
        return false;
    }
    return true;
}


template class IDbDoc<std::tuple<Id, Kw, Op>>;
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::tuple<Id, Kw, Op>>& iDbDoc);


/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/


const std::regex SrcIDb1Doc::FROM_STR_REGEX("\\((-?[0-9]+--?[0-9]+),(-?[0-9]+--?[0-9]+)\\)");


SrcIDb1Doc::SrcIDb1Doc(const Range<Kw>& kwRange, const Range<IdAlias>& idAliasRange) :
        IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>>(std::pair {kwRange, idAliasRange}) {}


std::string SrcIDb1Doc::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")";
    return ss.str();
}


SrcIDb1Doc SrcIDb1Doc::fromUstr(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, SrcIDb1Doc::FROM_STR_REGEX) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `SrcIDb1Doc.fromUstr()`, the world is going to end now" << std::endl;
        exit(EXIT_FAILURE);
    }
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    return SrcIDb1Doc {kwRange, idAliasRange};
}


template class IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>>;
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>>& iDbDoc);


/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/


template <class IndK, IDbDoc_ DbDoc>
void shuffleInd(Ind<IndK, DbDoc>& ind) {
    for (std::pair pair : ind) {
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), RNG);
    }
}


template <IDbDoc_ DbDoc, class DbKw>
Range<DbKw> findDbKwBounds(const Db<DbDoc, DbKw>& db) {
    if (db.empty()) {
        return DUMMY_RANGE<DbKw>();
    }

    DbKw minDbKw = DUMMY;
    DbKw maxDbKw = DUMMY;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        if (dbKwRange.first < minDbKw || minDbKw == DUMMY) {
            minDbKw = dbKwRange.first;
        }
        if (dbKwRange.second > maxDbKw || maxDbKw == DUMMY) {
            maxDbKw = dbKwRange.second;
        }
    }
    return Range {minDbKw, maxDbKw};
}


template <IDbDoc_ DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IDbDoc_ DbDoc>
void cleanUpResults(std::vector<DbDoc>& docs) {}


// template specialize just this method for `Doc` instead of all SSE classes that use it
template <>
void cleanUpResults(std::vector<Doc>& docs) {
    std::vector<Doc> newDocs;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Doc doc : docs) {
        Op op = doc.getOp();
        if (op == Op::DEL) {
            Id id = doc.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) docs
    for (Doc doc : docs) {
        Id id = doc.getId();
        Op op = doc.getOp();
        // make sure all dummy docs have `Op::DUMMY` so they are filtered out as well!
        if (op == Op::INS && deletedIds.count(id) == 0) {
            newDocs.push_back(doc);
        }
    }

    docs = newDocs;
}


template void shuffleInd(Ind<Kw, Doc>& ind);
template void shuffleInd(Ind<Kw, SrcIDb1Doc>& ind);
//template void shuffleInd(Ind<IdAlias, Doc>& ind);

template Range<Kw> findDbKwBounds(const Db<Doc, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Doc, Kw>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Doc, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Doc, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Doc, IdAlias>& db);

template void cleanUpResults(std::vector<Doc>& docs);
template void cleanUpResults(std::vector<SrcIDb1Doc>& docs);


/******************************************************************************/
/* Debugging                                                                  */
/******************************************************************************/


std::string strToHex(const uchar* str, int len) {
    std::stringstream ss;
    for (int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(str[i]) << " ";
    }
    return ss.str();
}


std::string strToHex(const ustring& str) {
    return strToHex(str.c_str(), str.length());
}
