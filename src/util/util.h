#pragma once

#include <concepts>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// todo range and iencryptable and tdag store their member vairable as reference?

////////////////////////////////////////////////////////////////////////////////
// Basic Declarations
////////////////////////////////////////////////////////////////////////////////

static const int KEY_SIZE   = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE    = 128 / 8;

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
using ustring = std::basic_string<unsigned char>;
using Kw      = int;

template <class T> class Range;
template <class T> class IDbDoc;
class IMainDbDoc;
class Id;
class Op;
class IdOp;
template <class DbKw = Kw> class SrciDb1Doc;

// black magic (https://stackoverflow.com/a/71921982)
// (java generics `extends`: look what they need to mimic a fraction of my power)
template <class T> concept IDbDocDeriv = requires(T t) {
    []<typename X>(IDbDoc<X>&){}(t);
};
template <class T> concept IMainDbDocDeriv = std::derived_from<T, IMainDbDoc>;

using IdRange    = Range<Id>;
// for generality, all keywords are ranges; single keywords are just size 1 ranges
using KwRange    = Range<Kw>;
//                `std::unordered_map<label, std::pair<data, iv>>`
using EncInd     = std::unordered_map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;
// allow polymorphic types for DB (id vs. (id, op) documents, Log-SRC-i etc.)
template <IDbDocDeriv DbDoc = IdOp, class DbKw = Kw> 
using Db         = std::vector<std::pair<DbDoc, Range<DbKw>>>;

////////////////////////////////////////////////////////////////////////////////
// `ustring`
////////////////////////////////////////////////////////////////////////////////

ustring toUstr(int n);
ustring toUstr(const std::string& s);
ustring toUstr(unsigned char* p, int len);
std::string fromUstr(const ustring& ustr);

// provide hash function for `ustring`s to use faster hashmap-based structures, like `unordered_map` instead of `map`
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

template <class T>
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
        template<class T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
};

// hash function
template<>
template<class T>
struct std::hash<Range<T>> {
    std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<std::string>{}(range.toStr());
    }
};

////////////////////////////////////////////////////////////////////////////////
// `IDbDoc`
////////////////////////////////////////////////////////////////////////////////

// interface for documents in dataset
template <class T>
class IDbDoc {
    protected:
        T val;

    public:
        IDbDoc() = default;
        IDbDoc(const T& val);

        T get() const;
        virtual ustring encode() const;
        virtual std::string toStr() const = 0;

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, const IDbDoc<T2>& iEncryptable);
};

class IMainDbDoc {
    public:
        virtual Id getId() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
// `Id`
////////////////////////////////////////////////////////////////////////////////

class Id : public IDbDoc<int>, public IMainDbDoc {
    public:
        Id() = default;
        Id(int val);

        static Id decode(const ustring& ustr);
        std::string toStr() const override;

        Id getId() const override;

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
};

// hash function
template<>
struct std::hash<Id> {
    std::size_t operator ()(const Id& id) const noexcept {
        return std::hash<std::string>{}(id.toStr());
    }
};

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
};

static const Op INSERT("INSERT");
static const Op DELETE("DELETE");

////////////////////////////////////////////////////////////////////////////////
// `IdOp`
////////////////////////////////////////////////////////////////////////////////

class IdOp : public IDbDoc<std::pair<Id, Op>>, public IMainDbDoc {
    public:
        IdOp() = default;
        IdOp(const Id& id);
        IdOp(const Id& id, const Op& op);

        static IdOp decode(const ustring& ustr);
        std::string toStr() const override;

        Id getId() const override;

        friend bool operator <(const IdOp& doc1, const IdOp& doc2);
        friend bool operator ==(const IdOp& doc1, const IdOp& doc2);
};

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

template <class DbKw>
class SrciDb1Doc : public IDbDoc<std::pair<Range<DbKw>, IdRange>> {
    public:
        SrciDb1Doc(const Range<DbKw>& dbKwRange, const IdRange& idRange);

        static SrciDb1Doc<DbKw> decode(const ustring& ustr);
        std::string toStr() const override;
};
