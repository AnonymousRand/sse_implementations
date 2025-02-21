#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

static const int KEY_SIZE   = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE    = 128 / 8;

// todo range and iencryptable and tdag store their member vairable as reference?

////////////////////////////////////////////////////////////////////////////////
// `ustring`
////////////////////////////////////////////////////////////////////////////////

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
using ustring = std::basic_string<unsigned char>;

ustring toUstr(int n);
ustring toUstr(const std::string& s);
ustring toUstr(unsigned char* p, int len);
std::string fromUstr(const ustring& ustr);

// provide hash function for `ustring`s to use `unordered_map`
// and other faster hashmap-based structures for them instead of `map`
template<>
struct std::hash<ustring> {
    std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(fromUstr(ustr));
    }
};

std::ostream& operator <<(std::ostream& os, const ustring& ustr);

////////////////////////////////////////////////////////////////////////////////
// `Range`
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

        std::string toStr() const;
        static Range<T> fromStr(const std::string& str);
        ustring toUstr() const;
        template<typename T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
        template<typename T2>
        // overload string concatenation (no way this worked)
        friend std::string operator +(const std::string& str, const Range<T2>& range);
};

// provide hash function as well
template<>
template<typename T>
struct std::hash<Range<T>> {
    std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<ustring>{}(range.toUstr());
    }
};

using Kw = int;
using KwRange = Range<Kw>;

////////////////////////////////////////////////////////////////////////////////
// `IEncryptable`
////////////////////////////////////////////////////////////////////////////////

// interface for documents in dataset
template <typename T>
class IEncryptable {
    protected:
        T val;

    public:
        IEncryptable() = default;
        IEncryptable(const T& val);

        T get() const;
        virtual ustring encode() const;
        virtual std::string toStr() const = 0;

        template <typename T2>
        friend std::ostream& operator <<(std::ostream& os, const IEncryptable<T2>& iEncryptable);
};

// todo is this used anywhere?
template <typename T>
ustring toUstr(const IEncryptable<T>& iEncryptable);

////////////////////////////////////////////////////////////////////////////////
// `Id`
////////////////////////////////////////////////////////////////////////////////

class Id : public IEncryptable<int> {
    public:
        Id() = default;
        Id(int val);

        static Id decode(const ustring& ustr);
        std::string toStr() const override;

        static Id getMin();
        static Id fromStr(const std::string& str);
        Id& operator ++();
        Id operator ++(int); // unused `int` param marks `++` as postfix
        Id& operator +=(const Id& id);
        Id& operator -=(const Id& id);
        Id& operator +=(int n);
        Id& operator -=(int n);
        friend Id operator +(Id id1, const Id& id2);
        friend Id operator +(Id id, int n);
        friend Id operator -(Id id1, const Id& id2);
        friend Id operator -(Id id, int n);
        friend bool operator ==(const Id& id1, const Id& id2);
        friend bool operator ==(const Id& id1, int n);
        friend bool operator <(const Id& id1, const Id& id2);
        friend bool operator >(const Id& id1, const Id& id2);
        friend bool operator <=(const Id& id1, const Id& id2);
        friend bool operator >=(const Id& id1, const Id& id2);
        friend std::string operator +(const std::string& str, const Id& id);
        friend std::string operator +(const Id& id, const std::string& str);
};

using IdRange = Range<Id>;

////////////////////////////////////////////////////////////////////////////////
// `Op`
////////////////////////////////////////////////////////////////////////////////

class Op {
    private:
        std::string val;

    public:
        Op() = default;
        Op(const std::string& val);

        std::string toStr() const;

        static Op fromStr(const std::string& val);
        friend bool operator ==(const Op& op1, const Op& op2);
        friend std::ostream& operator <<(std::ostream& os, const Op& Op);
        friend std::string operator +(const std::string& str, const Op& op);
};

static const Op INSERT("INSERT");
static const Op DELETE("DELETE");

////////////////////////////////////////////////////////////////////////////////
// `Doc`
////////////////////////////////////////////////////////////////////////////////

class Doc : public IEncryptable<std::pair<Id, Op>> {
    public:
        Doc() = default;
        Doc(const Id& id);
        Doc(const Id& id, const Op& op);

        static Doc decode(const ustring& ustr);
        std::string toStr() const override;

        static Doc getMin();
        friend bool operator <(const Doc& doc1, const Doc& doc2);
        friend bool operator ==(const Doc& doc1, const Doc& doc2);
};

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

// todo temp?
class SrciDb1Doc : public IEncryptable<std::pair<KwRange, IdRange>> {
    public:
        SrciDb1Doc(const KwRange& kwRange, const IdRange& idRange);

        std::string toStr() const override;
        static SrciDb1Doc decode(const ustring& ustr);
};

////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (screw you Log-SRC-i for making everything a nonillion times more complicated)
// no easy way to enforce templated base classes, like Java generics' `extends`
// so just make sure `DbDoc` subclasses `IEncryptable` and `DbKw` subclasses `Range`
template <typename DbDoc = Doc, typename DbKw = KwRange> 
using Db         = std::vector<std::pair<DbDoc, DbKw>>;
//                `std::unordered_map<label, std::pair<data, iv>>`
using EncInd     = std::unordered_map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;
