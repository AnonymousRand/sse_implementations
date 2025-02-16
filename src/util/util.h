#pragma once

#include <map>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

static const int KEY_SIZE   = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE    = 128 / 8;

////////////////////////////////////////////////////////////////////////////////
// ustring
////////////////////////////////////////////////////////////////////////////////

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
using ustring = std::basic_string<unsigned char>;

ustring toUstr(int n);
ustring toUstr(const std::string& s);
ustring toUstr(unsigned char* p, int len);
std::string fromUstr(const ustring& ustr);

std::ostream& operator <<(std::ostream& os, const ustring& ustr);

////////////////////////////////////////////////////////////////////////////////
// IEncryptable
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class IEncryptable {
    protected:
        T val;

    public:
        IEncryptable(const T& val);
        T get() const;

        virtual ustring encode() const = 0;
};

class Id : public IEncryptable<int> {
    public:
        Id(int val);

        ustring encode() const override;
        static Id decode(const ustring& ustr);

        friend Id abs(const Id& id);
        friend void operator ++(Id& id, int _); // unused `int` parameter required to mark `++` as postfix
        friend Id operator +(const Id& id1, const Id& id2);
        friend Id operator +(const Id& id, int n);
        friend Id operator -(const Id& id1, const Id& id2);
        friend Id operator -(const Id& id, int n);
        friend bool operator ==(const Id& id1, const Id& id2);
        friend bool operator <(const Id& id1, const Id& id2);
        friend bool operator >(const Id& id1, const Id& id2);
        friend bool operator <=(const Id& id1, const Id& id2);
        friend bool operator >=(const Id& id1, const Id& id2);
        friend std::ostream& operator <<(std::ostream& os, const Id& id);
};

template <typename T>
ustring toUstr(const IEncryptable<T>& iEncryptable);

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

// for generality, all keywords are ranges; single keywords are just size 1 ranges
template <typename T>
class Range : public std::pair<T, T> {
    public:
        Range();
        Range(const T& start, const T& end);

        T size() const;
        bool contains(const Range<T>& range) const;
        bool isDisjointWith(const Range<T>& range) const;

        static Range<T> fromStr(const std::string& str);

        template<typename T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
        template<typename T2> // overload string concatenation (no way this worked)
        friend std::string operator +(const std::string& str, const Range<T2>& range);
};

template <typename T>
ustring toUstr(const Range<T>& range);

using Kw      = int;
using IdRange = Range<Id>;
using KwRange = Range<Kw>;

// todo temp?

class SrciDb1Doc : public IEncryptable<std::pair<KwRange, IdRange>> {
    public:
        SrciDb1Doc(const KwRange& kwRange, const IdRange& idRange);

        ustring encode() const override;
        static SrciDb1Doc decode(const ustring& ustr);

        friend std::ostream& operator <<(std::ostream& os, const SrciDb1Doc& srciDb1Doc);
};

////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (screw you Log-SRC-i for making everything a nonillion times more complicated)
// no easy way to enforce (templated) base classes for clarity unfortunately, like Java generics `extends`
// so just make sure `DbDocType` subclasses `IEncryptable` and `DbKwType` subclasses `Range`
template <typename DbDocType = Id, typename DbKwType = KwRange>
using Db         = std::vector<std::tuple<DbDocType, DbKwType>>;
//                `std::map<label, std::pair<data, iv>>`
using EncInd     = std::map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;

template <typename DbDocType, typename DbKwType>
void sortDb(Db<DbDocType, DbKwType>& db);
