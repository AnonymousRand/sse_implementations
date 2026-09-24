#pragma once

#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


/**
 * interface for database tuples.
 *
 * note: we also store their `DbKw` range, which is the same as the size 1 range corresponding
 * to their `Kw` value for tuples inputted to the DB, but allows us to be general enough for
 * Log-SRC replications, for example, where this is not the case.
 */
template <IsDbDoc DbDoc, class DbKw>
struct IDbTuple {
public:
    using DbDocType = DbDoc;
    using DbKwType  = DbKw;

    DbDoc dbDoc;
    Range<DbKw> dbKwRange;

    // default constructor needed for children's default constructors
    IDbTuple() = default;
    IDbTuple(const DbDoc& dbDoc, const Range<DbKw>& dbKwRange) :
        dbDoc(dbDoc), dbKwRange(dbKwRange) {}

    ustring encode() const;
    // we pass in the return value as a param as `IDbTuple` is an abstract class,
    // so we can't return it by value; caller must instantiate a non-abstract child as `ret`
    // (and i don't wanna deal with pointers :3)
    static void decode(const ustring& encoding, IDbTuple& ret);
    virtual std::string toPrettyStr() const = 0;

    // (the `= default` seems to remove the need to template this friended method)
    friend bool operator ==(const IDbTuple& dbTuple1, const IDbTuple& dbTuple2) = default;
    template <IsDbDoc DbDoc2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbTuple<DbDoc2, DbKw2>& iDbTuple);
};


template <class T>
concept IsDbTuple = requires(T t) {
    []<class ... Args>(IDbTuple<Args ...>&){}(t);
};


//==============================================================================
// `Tuple`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw = Kw>
struct Tuple : public IDbTuple<Doc, DbKw> {
public:
    static Tuple DUMMY(const Range<DbKw>& dbKwRange) { return Tuple {Doc::DUMMY(), dbKwRange}; }
    bool isDummy() const { return *this == DUMMY(this->dbKwRange); }

    using IDbTuple<Doc, DbKw>::IDbTuple;
    // default constructor needed for caller of `decode()`
    Tuple() = default;
    Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    std::string toPrettyStr() const override;

    Id getId() const { return this->dbDoc.id; }
    Kw getKw() const { return this->dbDoc.kw; }
    Op getOp() const { return this->dbDoc.op; }
};


// specialize `std::hash` for `Tuple` so that they can be used as keys for `std::unordered_*`
template <class DbKw>
struct std::hash<Tuple<DbKw>> {
    inline std::size_t operator ()(const Tuple<DbKw>& tuple) const noexcept {
        return std::hash<std::string>{}(tuple.toStr());
    }
};


//==============================================================================
// `SrcIDb1Tuple`
//==============================================================================


struct SrcIDb1Tuple : public IDbTuple<SrcIDb1Doc, Kw> {
public:
    static SrcIDb1Tuple DUMMY(const Range<Kw>& kwRange) {
        return SrcIDb1Tuple {SrcIDb1Doc::DUMMY(), kwRange};
    }
    bool isDummy() const { return *this == DUMMY(this->dbKwRange); }

    using IDbTuple<SrcIDb1Doc, Kw>::IDbTuple;
    SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);

    std::string toPrettyStr() const override;

    Kw getKw() const { return this->dbDoc.kw; }
    const Range<IdAlias>& getIdAliasRange() const { return this->dbDoc.idAliasRange; }
};
