#pragma once

#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

static const int KEY_SIZE   = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE    = 128 / 8;

template <typename T, typename BaseClass>
concept Derives = std::is_base_of_v<BaseClass, T>;

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
// `IRangeable`
////////////////////////////////////////////////////////////////////////////////

class IRangeable {
    public:
        IRangeable() = default;
        virtual int getArith() const = 0;
        virtual void setArith(int val) = 0;

        // unused `int` param marks `++` as postfix
        virtual IRangeable& operator ++(int);
        virtual IRangeable& operator +=(const IRangeable& iRangeable);
        virtual IRangeable& operator -=(const IRangeable& iRangeable);
        virtual IRangeable& operator +(int n);
        virtual IRangeable& operator -(int n);
        friend bool operator ==(const IRangeable& iRangeable1, const IRangeable& iRangeable2);
        friend bool operator ==(const IRangeable& iRangeable1, int n);
        friend bool operator <(const IRangeable& iRangeable1, const IRangeable& iRangeable2);
        friend bool operator >(const IRangeable& iRangeable1, const IRangeable& iRangeable2);
        friend bool operator <=(const IRangeable& iRangeable1, const IRangeable& iRangeable2);
        friend bool operator >=(const IRangeable& iRangeable1, const IRangeable& iRangeable2);
        // overload string concatenation (no way this worked); unfortunately not symmetric overload
        friend std::string operator +(const std::string& str, const IRangeable& iRangeable);
        friend std::string operator +(const IRangeable& iRangeable, const std::string& str);
};

////////////////////////////////////////////////////////////////////////////////
// `Range`
////////////////////////////////////////////////////////////////////////////////

// for generality, all keywords are ranges; single keywords are just size 1 ranges
template <typename T> requires Derives<T, IRangeable>
// todo enforce subclassing as well for db
class Range : public std::pair<T, T> {
    public:
        Range();
        Range(const T& start, const T& end);

        T size() const;
        bool contains(const Range<T>& range) const;
        bool isDisjointWith(const Range<T>& range) const;

        std::string toStr() const;
        static Range<T> fromStr(const std::string& str);
        ustring toUstr() const; // todo is this ever used
        template<typename T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
        template<typename T2>
        friend std::string operator +(const std::string& str, const Range<T2>& range);
};

// provide hash function as well
template<>
template<typename T>
struct std::hash<Range<T>> {
    std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<ustring>{}(toUstr(range));
    }
};

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

class Id : public IEncryptable<int>, public IRangeable {
    public:
        Id() = default;
        Id(int val);

        static Id decode(const ustring& ustr);
        std::string toStr() const override;

        int getArith() const override;
        void setArith(int val) override;

        static Id fromStr(const std::string& str);
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
// `IdOp`
////////////////////////////////////////////////////////////////////////////////

class IdOp : public IEncryptable<std::pair<Id, Op>>, public IRangeable {
    public:
        IdOp() = default;
        IdOp(const Id& id);
        IdOp(const Id& id, const Op& op);

        static IdOp decode(const ustring& ustr);
        std::string toStr() const override;

        int getArith() const override;
        void setArith(int val) override;

        IdOp& operator ++(int) override;
        IdOp& operator +=(const IRangeable& iRangeable) override;
        IdOp& operator -=(const IRangeable& iRangeable) override;
        IdOp& operator +(int n) override;
        IdOp& operator -(int n) override;
        friend IdOp operator +(IdOp idOp1, const IdOp& idOp2);
        friend IdOp operator -(IdOp idOp1, const IdOp& idOp2);
};

using IdOpRange = Range<IdOp>;

IdRange toIdRange(const IdOpRange& idOpRange);

////////////////////////////////////////////////////////////////////////////////
// `Kw`
////////////////////////////////////////////////////////////////////////////////

class Kw : public IRangeable {
    private:
        int val;

    public:
        Kw() = default;
        Kw(int val);

        int getArith() const override;
        void setArith(int val) override;

        Kw& operator ++(int) override;
        Kw& operator +=(const IRangeable& iRangeable) override;
        Kw& operator -=(const IRangeable& iRangeable) override;
        Kw& operator +(int n) override;
        Kw& operator -(int n) override;
        friend Kw operator +(Kw kw1, const Kw& kw2);
        friend Kw operator -(Kw kw1, const Kw& kw2);
};

using KwRange = Range<Kw>;

////////////////////////////////////////////////////////////////////////////////
// `SrciDb1Doc`
////////////////////////////////////////////////////////////////////////////////

// todo temp?
template <typename DbDoc = IdOp>
class SrciDb1Doc : public IEncryptable<std::pair<KwRange, Range<DbDoc>>> {
    public:
        SrciDb1Doc(const KwRange& kwRange, const Range<DbDoc>& dbDocRange);

        std::string toStr() const override;
        static SrciDb1Doc<DbDoc> decode(const ustring& ustr);
};

////////////////////////////////////////////////////////////////////////////////
// Other
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (screw you Log-SRC-i for making everything a nonillion times more complicated)
// no easy way to enforce (templated) base classes for clarity, like Java generics' `extends`
// so just make sure `DbDoc` subclasses `IEncryptable` and `DbKw` subclasses `Range`
template <typename DbDoc = IdOp, typename DbKw = KwRange>
using Db         = std::vector<std::pair<DbDoc, DbKw>>;
//                `std::unordered_map<label, std::pair<data, iv>>`
using EncInd     = std::unordered_map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;

template <typename DbDoc, typename DbKw>
void sortDb(Db<DbDoc, DbKw>& db);
