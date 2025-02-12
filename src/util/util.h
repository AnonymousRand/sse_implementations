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
ustring toUstr(std::string s);
ustring toUstr(unsigned char* p, int len);
std::string fromUstr(ustring ustr);

std::ostream& operator <<(std::ostream& os, const ustring ustr);

////////////////////////////////////////////////////////////////////////////////
// IEncryptable
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class IEncryptable {
    protected:
        T val;

    public:
        IEncryptable(T val);
        T get();

        virtual ustring encode() = 0;
};

class Id : public IEncryptable<int> {
    public:
        Id(int val);

        ustring encode() override;
        static Id decode(ustring ustr);

        friend Id abs(const Id& id);
        friend void operator ++(Id& id, int _); // `int _` required to mark as postfix
        friend Id operator +(const Id& id1, const Id& id2);
        friend Id operator +(const Id& id, const int n);
        friend Id operator -(const Id& id1, const Id& id2);
        friend Id operator -(const Id& id, const int n);
        friend bool operator ==(const Id& id1, const Id& id2);
        friend bool operator <(const Id& id1, const Id& id2);
        friend bool operator >(const Id& id1, const Id& id2);
        friend bool operator <=(const Id& id1, const Id& id2);
        friend bool operator >=(const Id& id1, const Id& id2);
        friend std::ostream& operator <<(std::ostream& os, const Id& id);
};

template <typename T>
ustring toUstr(IEncryptable<T>& iEncryptable);

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

// for generality, all keywords are ranges; single keywords are just size 1 ranges
template <typename T>
class Range : public std::pair<T, T> {
    public:
        Range();
        Range(const T& start, const T& end);

        T size();
        bool contains(Range<T> range);
        bool isDisjointWith(Range<T> range);

        static Range<T> fromStr(std::string str);
        template<typename T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
        // overload string concatenation (no way this worked)
        template<typename T2>
        friend std::string operator +(const std::string& str, const Range<T2>& range);
};

using Kw      = int;
using IdRange = Range<Id>;
using KwRange = Range<Kw>;

template <typename T>
ustring toUstr(Range<T> range);

// todo temp?

class SrciDb1Doc : public IEncryptable<std::pair<KwRange, IdRange>> {
    public:
        SrciDb1Doc(KwRange kwRange, IdRange idRange);

        ustring encode() override;
        static SrciDb1Doc decode(ustring ustr);

        friend std::ostream& operator <<(std::ostream& os, const SrciDb1Doc& srciDb1Doc);
};

////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (because screw you Log-SRC-i for making everything a nonillion times more complicated)
// no easy way to enforce (templated) base classes for clarity unfortunately, like Java generics `extends`
// so just make sure `DbDocType` subclasses `IEncryptable` and `DbKwType` subclasses `Range`
template <typename DbDocType = Id, typename DbKwType = KwRange>
using Db         = std::vector<std::tuple<DbDocType, DbKwType>>; // todo test if list is faster
// changing `Doc` will lead to a snowball of broken dreams...
using Doc        = std::tuple<Id, KwRange>; // todo needed?? can just do make_tuple??
//                `std::map<label, std::pair<data, iv>>`
using EncInd     = std::map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;

template <typename DbDocType, typename DbKwType>
void sortDb(Db<DbDocType, DbKwType>& db);
