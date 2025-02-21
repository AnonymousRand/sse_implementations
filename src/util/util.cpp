#include <algorithm>
#include <cmath>
#include <regex>
#include <sstream>

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

template <typename T>
// can't call default constructor for `std::pair` without explicit vals? `0, 0` is supposed to be default
Range<T>::Range() : std::pair<T, T>(0, 0) {}

template <typename T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T>(start, end) {}

template <typename T>
T Range<T>::size() const {
    return (T)abs(this->second - this->first + 1);
}

template <typename T>
bool Range<T>::contains(const Range<T>& range) const {
    return this->first <= range.first && this->second >= range.second;
}

template <typename T>
bool Range<T>::isDisjointWith(const Range<T>& range) const {
    return this->second < range.first || this->first > range.second;
}

template <typename T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::regex re("(.*?)-(.*)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `Range.Range()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }

    Range<T> range;
    range.first = T(std::stoi(matches[1].str()));
    range.second = T(std::stoi(matches[2].str()));
    return range;
}

template <typename T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.first << "-" << range.second;
}

template<typename T>
std::string operator +(const std::string& str, const Range<T>& range) {
    std::stringstream ss;
    ss << str << range;
    return ss.str();
}

template <typename T>
ustring toUstr(const Range<T>& range) {
    return toUstr(range.first) + toUstr("-") + toUstr(range.second);
}

template class Range<Id>;
template class Range<Kw>;

template std::ostream& operator <<(std::ostream& os, const Range<Id>& range);
template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);

template std::string operator +(const std::string& str, const Range<Id>& range);
template std::string operator +(const std::string& str, const Range<Kw>& range);

template ustring toUstr(const Range<Id>& range);
template ustring toUstr(const Range<Kw>& range);

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

std::string operator +(const std::string& str, const Op& op) {
    return str + op.toStr();
}

////////////////////////////////////////////////////////////////////////////////
// `IEncryptable`
////////////////////////////////////////////////////////////////////////////////

template <typename T>
IEncryptable<T>::IEncryptable(const T& val) {
    this->val = val;
}

template <typename T>
T IEncryptable<T>::get() const {
    return this->val;
}

template <typename T>
ustring IEncryptable<T>::encode() const {
    return ::toUstr(this->toStr());
}

template <typename T>
std::ostream& operator <<(std::ostream& os, const IEncryptable<T>& iEncryptable) {
    return os << iEncryptable.toStr();
}

template <typename T>
ustring toUstr(const IEncryptable<T>& iEncryptable) {
    return iEncryptable.encode();
}

template class IEncryptable<int>;
template class IEncryptable<std::pair<Id, Op>>;
template class IEncryptable<std::pair<KwRange, IdRange>>;

template std::ostream& operator <<(std::ostream& os, const IEncryptable<int>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IEncryptable<std::pair<Id, Op>>& iEncryptable);
template std::ostream& operator <<(std::ostream& os, const IEncryptable<std::pair<KwRange, IdRange>>& iEncryptable);

template ustring toUstr(const IEncryptable<int>& id);
template ustring toUstr(const IEncryptable<std::pair<Id, Op>>& id);
template ustring toUstr(const IEncryptable<std::pair<KwRange, IdRange>>& srciDb1Doc);

////////////////////////////////////////////////////////////////////////////////
// `Id`
////////////////////////////////////////////////////////////////////////////////

Id::Id(int val) : IEncryptable<int>(val) {}

Id Id::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    return Id::fromStr(str);
}

std::string Id::toStr() const {
    return std::to_string(this->val);
}

Id Id::fromStr(const std::string& str) {
    return Id(std::stoi(str));
}

Id abs(const Id& id) {
    return Id(abs(id.val));
}

void operator ++(Id& id, int _) {
    id.val++;
}

Id operator +(const Id& id1, const Id& id2) {
    return id1.val + id2.val;
}

Id operator +(const Id& id1, int n) {
    return Id(id1.val + n);
}

Id operator -(const Id& id1, const Id& id2) {
    return id1.val - id2.val;
}

Id operator -(const Id& id1, int n) {
    return Id(id1.val - n);
}

bool operator ==(const Id& id1, const Id& id2) {
    return id1.val == id2.val;
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

std::string operator +(const std::string& str, const Id& id) {
    return str + id.toStr();
}

////////////////////////////////////////////////////////////////////////////////
// `IdOp`
////////////////////////////////////////////////////////////////////////////////

IdOp::IdOp(const Id& id, const Op& op) : IEncryptable<std::pair<Id, Op>>(std::pair {id, op}) {}

IdOp IdOp::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `IdOp.fromUstr()`, please panic calmly." << std::endl;
        exit(EXIT_FAILURE);
    }
    Id id = Id::fromStr(matches[1].str());
    Op op = Op::fromStr(matches[2].str());
    return IdOp {id, op};
}

std::string IdOp::toStr() const {
    return "(" + this->val.first + "," + this->val.second + ")";
}

bool operator <(const IdOp& idOp1, const IdOp& idOp2) {
    return idOp1.get().first < idOp2.get().first;
}

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

SrciDb1Doc::SrciDb1Doc(const KwRange& kwRange, const IdRange& idRange)
        : IEncryptable<std::pair<KwRange, IdRange>>(std::pair {kwRange, idRange}) {}

SrciDb1Doc SrciDb1Doc::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `SrciDb1Doc.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    KwRange kwRange = KwRange::fromStr(matches[1].str());
    IdRange idRange = IdRange::fromStr(matches[2].str());
    return SrciDb1Doc {kwRange, idRange};
}

std::string SrciDb1Doc::toStr() const {
    return "(" + this->val.first + "," + this->val.second + ")";
}

////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc, typename DbKw>
void sortDb(Db<DbDoc, DbKw>& db) {
    std::sort(
        db.begin(), db.end(),
        [](std::pair<DbDoc, DbKw> entry1, std::pair<DbDoc, DbKw> entry2
    ) {
        return entry1.first < entry2.first;
    });
}

template void sortDb(Db<Id, KwRange>& db);
