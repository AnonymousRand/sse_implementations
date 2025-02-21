#include <algorithm>
#include <regex>

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
// `IRangeable`
////////////////////////////////////////////////////////////////////////////////

IRangeable& IRangeable::operator ++(int) {
    this->setArith(this->getArith() + 1);
    return *this;
}

IRangeable& IRangeable::operator +=(const IRangeable& iRangeable) {
    this->setArith(this->getArith() + iRangeable.getArith());
    return *this;
}

IRangeable& IRangeable::operator -=(const IRangeable& iRangeable) {
    this->setArith(this->getArith() - iRangeable.getArith());
    return *this;
}

IRangeable& IRangeable::operator +(int n) {
    this->setArith(this->getArith() + n);
    return *this;
}

IRangeable& IRangeable::operator -(int n) {
    this->setArith(this->getArith() - n);
    return *this;
}

bool operator ==(const IRangeable& iRangeable1, const IRangeable& iRangeable2) {
    return iRangeable1.getArith() == iRangeable2.getArith();
}

bool operator ==(const IRangeable& iRangeable1, int n) {
    return iRangeable1.getArith() == n;
}

bool operator <(const IRangeable& iRangeable1, const IRangeable& iRangeable2) {
    return iRangeable1.getArith() < iRangeable2.getArith();
}

bool operator >(const IRangeable& iRangeable1, const IRangeable& iRangeable2) {
    return iRangeable1.getArith() > iRangeable2.getArith();
}

bool operator <=(const IRangeable& iRangeable1, const IRangeable& iRangeable2) {
    return iRangeable1.getArith() <= iRangeable2.getArith();
}

bool operator >=(const IRangeable& iRangeable1, const IRangeable& iRangeable2) {
    return iRangeable1.getArith() >= iRangeable2.getArith();
}

std::string operator +(const std::string& str, const IRangeable& iRangeable) {
    return str + std::to_string(iRangeable.getArith());
}

std::string operator +(const IRangeable& iRangeable, const std::string& str) {
    return std::to_string(iRangeable.getArith()) + str;
}

////////////////////////////////////////////////////////////////////////////////
// `Range`
////////////////////////////////////////////////////////////////////////////////

// can't call default constructor or set `= default` without explicit vals (ill-formed default definition apparently)
template <typename T>
Range<T>::Range() : std::pair<T, T>(T(), T()) {}

template <typename T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T> {start, end} {}

template <typename T>
T Range<T>::size() const {
    return this->second - this->first + 1;
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
std::string Range<T>::toStr() const {
    // todo may need reverse direction string concat overload in IRangeable
    return this->first + "-" + this->second;
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
ustring Range<T>::toUstr() const {
    return ::toUstr(this->toStr());
}

template <typename T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toStr();
}

template<typename T>
std::string operator +(const std::string& str, const Range<T>& range) {
    return str + range.toStr();
}

template class Range<Id>;
template class Range<IdOp>;
template class Range<Kw>;

template std::ostream& operator <<(std::ostream& os, const Range<Id>& range);
template std::ostream& operator <<(std::ostream& os, const Range<IdOp>& range);
template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);

template std::string operator +(const std::string& str, const Range<Id>& range);
template std::string operator +(const std::string& str, const Range<IdOp>& range);
template std::string operator +(const std::string& str, const Range<Kw>& range);

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

std::string Id::toStr() const {
    return std::to_string(this->val);
}

Id Id::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    return Id::fromStr(str);
}

int Id::getArith() const {
    return this->val;
}

void Id::setArith(int val) {
    this->val = val;
}

Id Id::fromStr(const std::string& str) {
    return Id(std::stoi(str));
}

Id operator +(Id id1, const Id& id2) {
    id1 += id2;
    return id1;
}

Id operator -(Id id1, const Id& id2) {
    id1 -= id2;
    return id1;
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

std::string operator +(const std::string& str, const Op& op) {
    return str + op.toStr();
}

////////////////////////////////////////////////////////////////////////////////
// `IdOp`
////////////////////////////////////////////////////////////////////////////////

IdOp::IdOp(const Id& id) : IEncryptable<std::pair<Id, Op>>(std::pair {id, INSERT}) {}

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

int IdOp::getArith() const {
    return this->val.first.getArith();
}

void IdOp::setArith(int val) {
    this->val.first = val;
}

IdOp operator +(IdOp idOp1, const IdOp& idOp2) {
    idOp1 += idOp2;
    return idOp1;
}

IdOp operator -(IdOp idOp1, const IdOp& idOp2) {
    idOp1 -= idOp2;
    return idOp1;
}

IdRange toIdRange(const IdOpRange& idOpRange) {
    std::pair startPair = idOpRange.first.get();
    std::pair endPair = idOpRange.second.get();
    return IdRange {startPair.first, endPair.first};
}

////////////////////////////////////////////////////////////////////////////////
// `Kw`
////////////////////////////////////////////////////////////////////////////////

Kw::Kw(int val) {
    this->val = val;
}

int Kw::getArith() const {
    return this->val;
}

void Kw::setArith(int val) {
    this->val = val;
}

Kw operator +(Kw kw1, const Kw& kw2) {
    kw1 += kw2;
    return kw1;
}

Kw operator -(Kw kw1, const Kw& kw2) {
    kw1 -= kw2;
    return kw1;
}

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

template <typename DbDoc>
SrciDb1Doc<DbDoc>::SrciDb1Doc(const KwRange& kwRange, const Range<DbDoc>& dbDocRange)
        : IEncryptable<std::pair<KwRange, Range<DbDoc>>>(std::pair {kwRange, dbDocRange}) {}

template <typename DbDoc>
SrciDb1Doc<DbDoc> SrciDb1Doc<DbDoc>::decode(const ustring& ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 3) {
        std::cerr << "Error: bad string passed to `SrciDb1Doc.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    KwRange kwRange = KwRange::fromStr(matches[1].str());
    Range<DbDoc> dbDocRange = Range<DbDoc>::fromStr(matches[2].str());
    return SrciDb1Doc {kwRange, dbDocRange};
}

template <typename DbDoc>
std::string SrciDb1Doc<DbDoc>::toStr() const {
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
