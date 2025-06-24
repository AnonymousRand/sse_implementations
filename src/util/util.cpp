#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_set>

#include "util.h"

/******************************************************************************/
/* `ustring`                                                                  */
/******************************************************************************/

ustring toUstr(int n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}

ustring toUstr(const std::string& s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring toUstr(unsigned char* p, int len) {
    return ustring(p, len);
}

std::string fromUstr(const ustring& ustr) {
    std::string str;
    for (unsigned char c : ustr) {
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
Range<T>::Range() : std::pair<T, T> {T(DUMMY), T(DUMMY)} {}

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
    std::regex re("(.*?)-(.*)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
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
template class Range<IdAlias>;

template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);

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
std::ostream& operator <<(std::ostream& os, const IDbDoc<T>& iEncryptable) {
    return os << iEncryptable.toStr();
}

template class IDbDoc<int>;
template class IDbDoc<std::pair<Id, Op>>;
template class IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>>;

template std::ostream& operator <<(std::ostream& os, const IDbDoc<Id>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<Id, Op>>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>>& iEncryptable);

template class IMainDbDoc<int>;
template class IMainDbDoc<std::pair<Id, Op>>;

/******************************************************************************/
/* `Doc`                                                                       */
/******************************************************************************/

Doc::Doc(int val) : IMainDbDoc<int>(val) {}

std::string Doc::toStr() const {
    return std::to_string(this->val);
}

Doc Doc::fromUstr(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    return Doc::fromStr(str);
}

Doc Doc::fromStr(const std::string& str) {
    return Doc(std::stoi(str));
}

Doc Doc::getDoc() const {
    return this->val;
}

bool operator ==(const Doc& doc1, const Doc& doc2) {
    return doc1.val == doc2.val;
}

/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/

template <class DbKw>
SrcIDb1Doc<DbKw>::SrcIDb1Doc(const Range<DbKw>& dbKwRange, const Range<IdAlias>& idAliasRange)
        : IDbDoc<std::pair<Range<DbKw>, Range<IdAlias>>>(std::pair {dbKwRange, idAliasRange}) {}

template <class DbKw>
SrcIDb1Doc<DbKw> SrcIDb1Doc<DbKw>::fromUstr(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `SrcIDb1Doc.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    return SrcIDb1Doc<DbKw> {kwRange, idAliasRange};
}

template <class DbKw>
std::string SrcIDb1Doc<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")";
    return ss.str();
}

template class SrcIDb1Doc<Kw>;

/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/

template <class DbKw, class DbDoc>
void shuffleInd(Ind<DbKw, DbDoc>& ind) {
    for (std::pair pair : ind) {
        std::vector<DbDoc> dbDocs = pair.second;
        std::shuffle(dbDocs.begin(), dbDocs.end(), RNG);
    }
}

template <class DbDoc, class DbKw>
DbKw findMaxDbKw(const Db<DbDoc, DbKw>& db) {
    DbKw maxDbKw = DbKw(DB_KW_MIN);
    if (!db.empty()) {
        Range<DbKw> firstDbKwRange = db[0].second;
        maxDbKw = firstDbKwRange.second;
        for (DbEntry<DbDoc, DbKw> entry : db) {
            Range<DbKw> dbKwRange = entry.second;
            if (dbKwRange.second > maxDbKw) {
                maxDbKw = dbKwRange.second;
            }
        }
    }
    return maxDbKw;
}

template <class DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db) {
    std::unordered_set<Range<DbKw>> uniqDbKwRanges;
    for (DbEntry<DbDoc, DbKw> entry : db) {
        Range<DbKw> dbKwRange = entry.second;
        uniqDbKwRanges.insert(dbKwRange); // `unordered_set` will not insert duplicate elements
    }
    return uniqDbKwRanges;
}

template void shuffleInd(Ind<Kw, Id>& ind);
template void shuffleInd(Ind<Kw, IdOp>& ind);
template void shuffleInd(Ind<IdAlias, Id>& ind);
template void shuffleInd(Ind<IdAlias, IdOp>& ind);
template void shuffleInd(Ind<Kw, SrcIDb1Doc<Kw>>& ind);

template Kw findMaxDbKw(const Db<Id, Kw>& db);
template Kw findMaxDbKw(const Db<IdOp, Kw>& db);

template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<Id, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<IdOp, Kw>& db);
template std::unordered_set<Range<Kw>> getUniqDbKwRanges(const Db<SrcIDb1Doc<Kw>, Kw>& db);
template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<Id, IdAlias>& db);
template std::unordered_set<Range<IdAlias>> getUniqDbKwRanges(const Db<IdOp, IdAlias>& db);
