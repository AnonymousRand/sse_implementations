// >>TODO: eventually make Db also be stored on disk like EncInds?

#pragma once

#include <concepts>
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


/**
 * interface for database tuples.
 *
 * note that we also store their `DbKw` range, which is the same as the size 1 range corresponding
 * to their `Kw` value for tuples inputted to the DB, but allows us to be general enough for
 * Log-SRC replications, for example, where this is not the case. we need to be able to easily
 * fetch this in plaintext for things like SDa (otherwise it might be only accessible via the
 * encrypted "label" in the encrypted index, which can be a hash/PRF and hence not easily
 * reversible, unlike `DbTuple`s which are just encrypted and can be easily decrypted).
 */
template <class DbDoc, class DbKw>
class IDbTuple {
public:
    using DbDocType = DbDoc;
    using DbKwType  = DbKw;

    IDbTuple(const DbDoc& val, const Range<DbKw>& dbKwRange);

    DbDoc getDbDoc() const;
    Range<DbKw> getDbKwRange() const;

    virtual std::string toStr() const = 0;
    ustring toUstr() const;

    template <class DbDoc2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbTuple<DbDoc2, DbKw2>& iDbTuple);

protected:
    DbDoc dbDoc;
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


//==============================================================================
// `Tuple`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw = Kw>
class Tuple : public IDbTuple<std::tuple<Id, Kw, Op>, DbKw> {
public:
    using IDbTuple<std::tuple<Id, Kw, Op>, DbKw>::IDbTuple;

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
    inline std::size_t operator ()(const Tuple<DbKw>& tuple) const noexcept {
        return std::hash<std::string>{}(tuple.toStr());
    }
};


//==============================================================================
// `SrcIDb1Tuple`
//==============================================================================


class SrcIDb1Tuple : public IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw> {
public:
    using IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw>::IDbTuple;

    SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);

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
// `Db` and `Ind`
//==============================================================================


template <IsDbTuple DbTuple = Tuple<>>
using Db = std::vector<DbTuple>;

template <IsDbTuple DbTuple = Tuple<>>
using Ind = std::unordered_map<Range<typename DbTuple::DbKwType>, Db<DbTuple>>;
