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
// `IDbEntry`
//==============================================================================


// interface for documents in dataset; also store the `DbKw` range (e.g. Log-SRC replications)
// they're stored with in their respective datasets, so that we can easily fetch them in
// plaintext for things like SDa (otherwise they might be only accessible via the
// encrypted "label" in the encrypted index, which can be a hash/PRF and hence not easily
// reversible, unlike `DbEntry`s which are just encrypted and can be easily decrypted)
template <class T, class DbKw>
class IDbEntry {
public:
    IDbEntry() = default;
    IDbEntry(const T& val, const Range<DbKw>& dbKwRange);

    T get() const;
    Range<DbKw> getDbKwRange() const;

    virtual std::string toStr() const = 0;
    ustring toUstr() const;

    template <class T2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbEntry<T2, DbKw2>& iDbEntry);

protected:
    T val;
    Range<DbKw> dbKwRange;
};


// black magic to detect if `T` is derived from `IDbEntry` regardless of `IDbEntry`'s
// template param, i.e. without needing to know what the template param `T2` of
// `IDbEntry` is, unlike `std::derived_from` for example (Java generics `extends`:
// look what they need to mimic a fraction of my power) (and this doesn't even enforce
// existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IsDbEntry = requires(T t) {
    []<class ... Args>(IDbEntry<Args ...>&){}(t);
};


// this enforces the above plus that `T` uses `DbKw` as its second template param, e.g.
// `IsDbEntry<IDbEntry<A, Kw>, Kw>` passes but not `IsDbEntry<IDbEntry<A, Kw>, char>`
template <class DbEntry, class DbKw>
concept IsValidDbParams = requires(DbEntry t) {
    []<class T2>(IDbEntry<T2, DbKw>&){}(t);
};


//==============================================================================
// `DefaultDbEntry`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw = Kw>
class DefaultDbEntry : public IDbEntry<std::tuple<Id, Kw, Op>, DbKw> {
public:
    DefaultDbEntry() = default;
    DefaultDbEntry(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    Id getId() const;
    Kw getKw() const;
    Op getOp() const;

    std::string toStr() const override;
    static DefaultDbEntry<DbKw> fromUstr(const ustring& ustr);
    static DefaultDbEntry<DbKw> genDummy(const Range<DbKw>& dbKwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


template <class DbKw>
struct std::hash<DefaultDbEntry<DbKw>> {
    inline std::size_t operator ()(const DefaultDbEntry<DbKw>& defaultDbEntry) const noexcept {
        return std::hash<std::string>{}(defaultDbEntry.toStr());
    }
};


//==============================================================================
// `SrcIDb1Entry`
//==============================================================================


class SrcIDb1Entry : public IDbEntry<std::pair<Kw, Range<IdAlias>>, Kw> {
public:
    SrcIDb1Entry() = default;
    SrcIDb1Entry(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);
    SrcIDb1Entry(const SrcIDb1Entry& srcIDb1Entry);

    Kw getKw() const;
    Range<IdAlias> getIdAliasRange() const;

    std::string toStr() const override;
    static SrcIDb1Entry fromUstr(const ustring& ustr);
    static SrcIDb1Entry genDummy(const Range<Kw>& kwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


//==============================================================================
// `Ind`
//==============================================================================


template <class DbEntry = DefaultDbEntry<>, class DbKw = Kw>
using Ind = std::unordered_map<Range<DbKw>, std::vector<DbEntry>>;
