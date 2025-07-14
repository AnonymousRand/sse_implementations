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


std::string toStr(const ustring& ustr) {
    std::string str;
    for (uchar c : ustr) {
        str += static_cast<char>(c);
    }
    return str;
}


std::ostream& operator <<(std::ostream& os, const ustring& ustr) {
    return os << toStr(ustr);
}


/******************************************************************************/
/* `Range`                                                                    */
/******************************************************************************/


template <class T>
const std::string Range<T>::REGEX_STR = "(-?[0-9]+)-(-?[0-9]+)";


template <class T>
const std::regex Range<T>::REGEX(Range<T>::REGEX_STR);


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
ustring Range<T>::toUstr() const {
    return ::toUstr(this->toStr());
}


template <class T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, Range<T>::REGEX) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `Range.fromStr()`, the world is going to end" << std::endl;
        std::cerr << "Regex to match is \"" << Range<T>::REGEX_STR << "\"; matched groups are:" << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    Range<T> range;
    range.first = T(std::stoi(matches[1].str()));
    range.second = T(std::stoi(matches[2].str()));
    return range;
}


template <class T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toStr();
}


template class Range<Kw>;
//template class Range<IdAlias>;

template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
//template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);


/******************************************************************************/
/* `IDbDoc`                                                                   */
/******************************************************************************/


template <class T, class DbKw>
IDbDoc<T, DbKw>::IDbDoc(const T& val, const Range<DbKw>& dbKwRange) {
    this->val = val;
    this->dbKwRange = dbKwRange;
}


template <class T, class DbKw>
T IDbDoc<T, DbKw>::get() const {
    return this->val;
}


template <class T, class DbKw>
Range<DbKw> IDbDoc<T, DbKw>::getDbKwRange() const {
    return this->dbKwRange;
}


template <class T, class DbKw>
ustring IDbDoc<T, DbKw>::toUstr() const {
    return ::toUstr(this->toStr());
}


template <class T, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbDoc<T, DbKw>& iDbDoc) {
    return os << iDbDoc.toStr();
}


/******************************************************************************/
/* `Doc`                                                                      */
/******************************************************************************/


template <class DbKw>
const std::string Doc<DbKw>::REGEX_STR = "\\(\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\),(-?[0-9]+--?[0-9]+)\\)";


template <class DbKw>
const std::regex Doc<DbKw>::REGEX(Doc<DbKw>::REGEX_STR);


template <class DbKw>
Doc<DbKw>::Doc(const std::tuple<Id, Kw, Op>& val, const Range<DbKw>& dbKwRange) :
        IDbDoc<std::tuple<Id, Kw, Op>, DbKw>(val, dbKwRange) {}


template <class DbKw>
Doc<DbKw>::Doc(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) : Doc<DbKw>(std::tuple {id, kw, op}, dbKwRange) {}


template <class DbKw>
std::string Doc<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "((" << this->getId() << "," << this->getKw() << "," << static_cast<char>(this->getOp()) << "),"
       << this->dbKwRange << ")";
    return ss.str();
}


template <class DbKw>
Doc<DbKw> Doc<DbKw>::fromUstr(const ustring& ustr) {
    std::string str = ::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, Doc<DbKw>::REGEX) || matches.size() != 5) {
        std::cerr << "Error: bad string \"" << str << "\" passed to `Doc.fromUstr()`, the world is going to end now"
                  << std::endl;
        std::cerr << "Regex to match is \"" << Doc<DbKw>::REGEX_STR << "\"; matched groups are:" << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        exit(EXIT_FAILURE);
    }
    Id id = std::stoi(matches[1].str());
    Kw kw = std::stoi(matches[2].str());
    Op op = static_cast<Op>(matches[3].str()[0]);
    Range<DbKw> dbKwRange = Range<DbKw>::fromStr(matches[4].str());
    return Doc<DbKw> {id, kw, op, dbKwRange};
}


template <class DbKw>
Id Doc<DbKw>::getId() const {
    return std::get<0>(this->val);
}


template <class DbKw>
Kw Doc<DbKw>::getKw() const {
    return std::get<1>(this->val);
}


template <class DbKw>
Op Doc<DbKw>::getOp() const {
    return std::get<2>(this->val);
}


template class IDbDoc<std::tuple<Id, Kw, Op>, Kw>;
//template class IDbDoc<std::tuple<Id, Kw, Op>, IdAlias>;
template class Doc<Kw>;
//template class Doc<IdAlias>;

template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::tuple<Id, Kw, Op>, Kw>& iDbDoc);
//template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::tuple<Id, Kw, Op>, IdAlias>& iDbDoc);


/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/


const std::string SrcIDb1Doc::REGEX_STR = "\\(\\((-?[0-9]+),(-?[0-9]+--?[0-9]+)\\),(-?[0-9]+--?[0-9]+)\\)";


const std::regex SrcIDb1Doc::REGEX(SrcIDb1Doc::REGEX_STR);


SrcIDb1Doc::SrcIDb1Doc(const std::pair<Kw, Range<IdAlias>>& val, const Range<Kw>& kwRange) :
        IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>(val, kwRange) {}


SrcIDb1Doc::SrcIDb1Doc(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
        SrcIDb1Doc(std::pair {kw, idAliasRange}, kwRange) {}


std::string SrcIDb1Doc::toStr() const {
    std::stringstream ss;
    ss << "((" << this->val.first << "," << this->val.second << ")," << this->dbKwRange << ")";
    return ss.str();
}


SrcIDb1Doc SrcIDb1Doc::fromUstr(const ustring& ustr) {
    std::string str = ::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, SrcIDb1Doc::REGEX) || matches.size() != 4) {
        std::cerr << "Error: bad string \"" << ustr
                  << "\" passed to `SrcIDb1Doc.fromUstr()`, the world is going to end now" << std::endl;
        std::cerr << "Regex to match is \"" << SrcIDb1Doc::REGEX_STR << "\"; matched groups are:" << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        exit(EXIT_FAILURE);
    }
    Kw kw = std::stol(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[3].str());
    return SrcIDb1Doc {kw, idAliasRange, kwRange};
}


template class IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>;

template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>& iDbDoc);


/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/


template <class IndK, IsDbDoc DbDoc>
void shuffleInd(Ind<IndK, DbDoc>& ind) {
    for (std::pair pair : ind) {
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), RNG);
    }
}


template <IsDbDoc DbDoc, class DbKw>
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


template <IsDbDoc DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}


template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& docs) {}


// template specialize just this method for `Doc<>` instead of all SSE classes that use it
template <>
void cleanUpResults(std::vector<Doc<>>& docs) {
    std::vector<Doc<>> newDocs;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (Doc<> doc : docs) {
        Op op = doc.getOp();
        if (op == Op::DEL) {
            Id id = doc.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) docs
    for (Doc<> doc : docs) {
        Id id = doc.getId();
        Op op = doc.getOp();
        // make sure all dummy docs have `Op::DUMMY` so they are filtered out as well!
        if (op == Op::INS && deletedIds.count(id) == 0) {
            newDocs.push_back(doc);
        }
    }

    docs = newDocs;
}


template void shuffleInd(Ind<Kw, Doc<>>& ind);
template void shuffleInd(Ind<Kw, SrcIDb1Doc>& ind);
//template void shuffleInd(Ind<IdAlias, Doc<IdAlias>>& ind);

template Range<Kw> findDbKwBounds(const Db<Doc<>, Kw>& db);
template Range<Kw> findDbKwBounds(const Db<SrcIDb1Doc, Kw>& db);
//template Range<Kw> findDbKwBounds(const Db<Doc<IdAlias>, IdAlias>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Doc<>, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Doc, Kw>& db);
//template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Doc<IdAlias>, IdAlias>& db);

template void cleanUpResults(std::vector<Doc<>>& docs);
template void cleanUpResults(std::vector<SrcIDb1Doc>& docs);
//template void cleanUpResults(std::vector<Doc<IdAlias>>& docs);


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
