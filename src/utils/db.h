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
// `IDbRecord`
//==============================================================================


// interface for documents in dataset; also store the `DbKw` range (e.g. Log-SRC replications)
// they're stored with in their respective datasets, so that we can easily fetch them in
// plaintext for things like SDa (otherwise they might be only accessible via the
// encrypted "label" in the encrypted index, which can be a hash/PRF and hence not easily
// reversible, unlike `DbRecord`s which are just encrypted and can be easily decrypted)
template <class T, class DbKw>
class IDbRecord {
public:
    IDbRecord() = default;
    IDbRecord(const T& val, const Range<DbKw>& dbKwRange);

    T get() const;
    Range<DbKw> getDbKwRange() const;

    virtual std::string toStr() const = 0;
    ustring toUstr() const;

    template <class T2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbRecord<T2, DbKw2>& iDbRecord);

protected:
    T val;
    Range<DbKw> dbKwRange;
};


// black magic to detect if `T` is derived from `IDbRecord` regardless of `IDbRecord`'s
// template param, i.e. without needing to know what the template param `T2` of
// `IDbRecord` is, unlike `std::derived_from` for example (Java generics `extends`:
// look what they need to mimic a fraction of my power) (and this doesn't even enforce
// existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IsDbRecord = requires(T t) {
    []<class ... Args>(IDbRecord<Args ...>&){}(t);
};


// this enforces the above plus that `T` uses `DbKw` as its second template param, e.g.
// `IsDbRecord<IDbRecord<A, Kw>, Kw>` passes but not `IsDbRecord<IDbRecord<A, Kw>, char>`
template <class DbRecord, class DbKw>
concept IsValidDbParams = requires(DbRecord t) {
    []<class T2>(IDbRecord<T2, DbKw>&){}(t);
};


//==============================================================================
// `Record`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw = Kw>
class Record : public IDbRecord<std::tuple<Id, Kw, Op>, DbKw> {
public:
    Record() = default;
    Record(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    Id getId() const;
    Kw getKw() const;
    Op getOp() const;

    std::string toStr() const override;
    static Record<DbKw> fromUstr(const ustring& ustr);
    static Record<DbKw> genDummy(const Range<DbKw>& dbKwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


template <class DbKw>
struct std::hash<Record<DbKw>> {
    inline std::size_t operator ()(const Record<DbKw>& defaultDbRecord) const noexcept {
        return std::hash<std::string>{}(defaultDbRecord.toStr());
    }
};


//==============================================================================
// `SrcIDb1Record`
//==============================================================================


class SrcIDb1Record : public IDbRecord<std::pair<Kw, Range<IdAlias>>, Kw> {
public:
    SrcIDb1Record() = default;
    SrcIDb1Record(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);
    SrcIDb1Record(const SrcIDb1Record& srcIDb1Record);

    Kw getKw() const;
    Range<IdAlias> getIdAliasRange() const;

    std::string toStr() const override;
    static SrcIDb1Record fromUstr(const ustring& ustr);
    static SrcIDb1Record genDummy(const Range<Kw>& kwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


//==============================================================================
// `Ind`
//==============================================================================


template <class DbRecord = Record<>, class DbKw = Kw>
using Ind = std::unordered_map<Range<DbKw>, std::vector<DbRecord>>;
