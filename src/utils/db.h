// TODO: eventually make Db also be stored on disk like EncInds? and move benchmarks
// from server to Db and EncInds and make them keep track of disk size

#pragma once

// TODO what is cstddef for
#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/range.h"
#include "utils/types.h"
#include "utils/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


// interface for documents in dataset; also store the `DbKw` range (e.g. Log-SRC replications)
// they're stored with in their respective datasets, so that we can easily fetch them in
// plaintext for things like SDa (otherwise they might be only accessible via the
// encrypted "label" in the encrypted index, which can be a hash/PRF and hence not easily
// reversible, unlike `DbTuple`s which are just encrypted and can be easily decrypted)
template <class T, class DbKw>
class IDbTuple {
public:
    IDbTuple() = default;
    IDbTuple(const T& val, const Range<DbKw>& dbKwRange);

    T get() const;
    Range<DbKw> getDbKwRange() const;

    virtual std::string toStr() const = 0;
    ustring toUstr() const;

    template <class T2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbTuple<T2, DbKw2>& iDbTuple);

protected:
    T val;
    Range<DbKw> dbKwRange;
};


// black magic to detect if `T` is derived from `IDbTuple` regardless of `IDbTuple`'s
// template param, i.e. without needing to know what the template param `T2` of
// `IDbTuple` is, unlike `std::derived_from` for example (Java generics `extends`:
// look what they need to mimic a fraction of my power) (and this doesn't even enforce
// existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IsDbTuple = requires(T t) {
    []<class ... Args>(IDbTuple<Args ...>&){}(t);
};


// this enforces the above plus that `T` uses `DbKw` as its second template param, e.g.
// `IsDbTuple<IDbTuple<A, Kw>, Kw>` passes but not `IsDbTuple<IDbTuple<A, Kw>, char>`
template <class DbTuple, class DbKw>
concept IsValidDbParams = requires(DbTuple t) {
    []<class T2>(IDbTuple<T2, DbKw>&){}(t);
};


//==============================================================================
// `Tuple`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw = Kw>
class Tuple : public IDbTuple<std::tuple<Id, Kw, Op>, DbKw> {
public:
    Tuple() = default;
    Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    Id getId() const;
    Kw getKw() const;
    Op getOp() const;

    std::string toStr() const override;
    static Tuple<DbKw> fromUstr(const ustring& ustr);
    static Tuple<DbKw> genDummy(const Range<DbKw>& dbKwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


template <class DbKw>
struct std::hash<Tuple<DbKw>> {
    inline std::size_t operator ()(const Tuple<DbKw>& defaultDbTuple) const noexcept {
        return std::hash<std::string>{}(defaultDbTuple.toStr());
    }
};


//==============================================================================
// `SrcIDb1Tuple`
//==============================================================================


class SrcIDb1Tuple : public IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw> {
public:
    SrcIDb1Tuple() = default;
    SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);
    SrcIDb1Tuple(const SrcIDb1Tuple& srcIDb1Tuple);

    Kw getKw() const;
    Range<IdAlias> getIdAliasRange() const;

    std::string toStr() const override;
    static SrcIDb1Tuple fromUstr(const ustring& ustr);
    static SrcIDb1Tuple genDummy(const Range<Kw>& kwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


//==============================================================================
// `Ind`
//==============================================================================


template <class DbTuple = Tuple<>, class DbKw = Kw>
using Ind = std::unordered_map<Range<DbKw>, std::vector<DbTuple>>;
