#include <regex>
#include <sstream>
#include <unordered_set>

#include "util.h"

////////////////////////////////////////////////////////////////////////////////
// `ustring`
////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
// `Range`
////////////////////////////////////////////////////////////////////////////////

// can't call default constructor or set `= default` without explicit vals (ill-formed default definition apparently)
template <class T>
Range<T>::Range() : std::pair<T, T> {T(), T()} {}

template <class T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T> {start, end} {}

template <class T>
T Range<T>::size() const {
    return this->second - this->first + 1;
}

template <class T>
bool Range<T>::contains(const Range<T>& range) const {
    return this->first <= range.first && this->second >= range.second;
}

template <class T>
bool Range<T>::isDisjointWith(const Range<T>& range) const {
    return this->second < range.first || this->first > range.second;
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

template class Range<Id>;
template class Range<Kw>;

template std::ostream& operator <<(std::ostream& os, const Range<Id>& range);
template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);

////////////////////////////////////////////////////////////////////////////////
// `IDbDoc`
////////////////////////////////////////////////////////////////////////////////

template <class T>
IDbDoc<T>::IDbDoc(const T& val) {
    this->val = val;
}

template <class T>
T IDbDoc<T>::get() const {
    return this->val;
}

template <class T>
ustring IDbDoc<T>::encode() const {
    return ::toUstr(this->toStr());
}

template <class T>
std::ostream& operator <<(std::ostream& os, const IDbDoc<T>& iEncryptable) {
    return os << iEncryptable.toStr();
}

template class IDbDoc<int>;
template class IDbDoc<std::pair<Id, Op>>;
template class IDbDoc<std::pair<KwRange, IdRange>>;

template std::ostream& operator <<(std::ostream& os, const IDbDoc<int>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<Id, Op>>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IDbDoc<std::pair<KwRange, IdRange>>& iEncryptable);

template class IMainDbDoc<int>;
template class IMainDbDoc<std::pair<Id, Op>>;

////////////////////////////////////////////////////////////////////////////////
// `Id`
////////////////////////////////////////////////////////////////////////////////

Id::Id(int val) : IMainDbDoc<int>(val) {}

std::string Id::toStr() const {
    return std::to_string(this->val);
}

Id Id::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    return Id::fromStr(str);
}

Id Id::fromStr(const std::string& str) {
    return Id(std::stoi(str));
}

Id Id::getId() const {
    return this->val;
}

Id& Id::operator ++() {
    this->val = ++(this->val);
    return *this;
}

Id Id::operator ++(int) {
    Id old = *this;
    ++(*this);
    return old;
}

Id& Id::operator +=(const Id& id) {
    this->val += id.val;
    return *this;
}

Id& Id::operator -=(const Id& id) {
    this->val -= id.val;
    return *this;
}

Id& Id::operator +=(int n) {
    this->val += n;
    return *this;
}

Id& Id::operator -=(int n) {
    this->val -= n;
    return *this;
}

Id operator +(Id id1, const Id& id2) {
    id1 += id2;
    return id1;
}

Id operator +(Id id, int n) {
    id += n;
    return id;
}

Id operator -(Id id1, const Id& id2) {
    id1 -= id2;
    return id1;
}

Id operator -(Id id, int n) {
    id -= n;
    return id;
}

bool operator ==(const Id& id1, const Id& id2) {
    return id1.val == id2.val;
}

bool operator ==(const Id& id1, int n) {
    return id1.val == n;
}

bool operator <(const Id& id1, const Id& id2) {
    return id1.val < id2.val;
}

bool operator >(const Id& id1, const Id& id2) {
    return id1.val > id2.val;
}

bool operator <=(const Id& id1, const Id& id2) {
    return id1.val <= id2.val;
}

bool operator >=(const Id& id1, const Id& id2) {
    return id1.val >= id2.val;
}

////////////////////////////////////////////////////////////////////////////////
// `Op`
////////////////////////////////////////////////////////////////////////////////

Op::Op(const std::string& val) {
    this->val = val;
}

std::string Op::toStr() const {
    return this->val;
}

Op Op::fromStr(const std::string& val) {
    return Op(val);
}

bool operator ==(const Op& op1, const Op& op2) {
    return op1.toStr() == op2.toStr();
}

std::ostream& operator <<(std::ostream& os, const Op& op) {
    return os << op.toStr();
}

////////////////////////////////////////////////////////////////////////////////
// `IdOp`
////////////////////////////////////////////////////////////////////////////////

IdOp::IdOp(const Id& id) : IMainDbDoc<std::pair<Id, Op>>(std::pair {id, INSERT}) {}

IdOp::IdOp(const Id& id, const Op& op) : IMainDbDoc<std::pair<Id, Op>>(std::pair {id, op}) {}

IdOp IdOp::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `IdOp.fromUstr()`, please panic (calmly)." << std::endl;
        exit(EXIT_FAILURE);
    }
    Id id = Id::fromStr(matches[1].str());
    Op op = Op::fromStr(matches[2].str());
    return IdOp {id, op};
}

std::string IdOp::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")";
    return ss.str();
}

Id IdOp::getId() const {
    return this->val.first;
}

bool operator <(const IdOp& doc1, const IdOp& doc2) {
    return doc1.val.first < doc2.val.first;
}

bool operator ==(const IdOp& doc1, const IdOp& doc2) {
    return doc1.val.first == doc2.val.first;
}

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

template <class DbKw>
SrciDb1Doc<DbKw>::SrciDb1Doc(const Range<DbKw>& dbKwRange, const IdRange& idRange)
        : IDbDoc<std::pair<Range<DbKw>, IdRange>>(std::pair {dbKwRange, idRange}) {}

template <class DbKw>
SrciDb1Doc<DbKw> SrciDb1Doc<DbKw>::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `SrciDb1Doc.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    KwRange kwRange = KwRange::fromStr(matches[1].str());
    IdRange idRange = IdRange::fromStr(matches[2].str());
    return SrciDb1Doc<DbKw> {kwRange, idRange};
}

template <class DbKw>
std::string SrciDb1Doc<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")";
    return ss.str();
}

template class SrciDb1Doc<Kw>;
