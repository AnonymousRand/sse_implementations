#pragma once

#include <concepts>
#include <cstddef>
#include <iostream>
#include <regex>
#include <string>

#include "types/basic_types.h"
#include "types/doc.h"
#include "types/range.h"
#include "types/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


/**
 * interface for database tuples.
 *
 * note: we also store their `DbKw` range, which is the same as the size 1 range corresponding
 * to their `Kw` value for tuples inputted to the DB, but allows us to be general enough for
 * Log-SRC replications, for example, where this is not the case. we need to be able to easily
 * fetch this in plaintext for things like SDa (otherwise it might be only accessible via the
 * encrypted "label" in the encrypted index, which can be a hash/PRF and hence not easily
 * reversible, unlike `DbTuple`s which are just encrypted and can be easily decrypted).
 */
template <IsDbDoc DbDoc, class DbKw>
struct IDbTuple {
public:
    using DbDocType = DbDoc;
    using DbKwType  = DbKw;

    DbDoc dbDoc;
    Range<DbKw> dbKwRange;

    IDbTuple(const DbDoc& dbDoc, const Range<DbKw>& dbKwRange) :
        dbDoc(dbDoc), dbKwRange(dbKwRange) {}

    virtual std::string toStr() const = 0;
    virtual std::string toPrintableStr() const = 0;
    ustring toUstr() const;

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
    static const Tuple DUMMY(const Range<DbKw>& dbKwRange) {
        return Tuple {Doc::DUMMY(), dbKwRange};
    }
    const bool isDummy() const {
        return *this == DUMMY(this->dbKwRange);
    }

    //--------------------------------------------------------------------------
    // `IDbTuple`

    using IDbTuple<Doc, DbKw>::IDbTuple;
    Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    std::string toStr() const override;
    std::string toPrintableStr() const override;
    static Tuple fromStr(const std::string& str);
    static Tuple fromUstr(const ustring& ustr);

    Id getId() const { return this->dbDoc.id; }
    Kw getKw() const { return this->dbDoc.kw; }
    Op getOp() const { return this->dbDoc.op; }

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
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
    static const SrcIDb1Tuple DUMMY(const Range<Kw>& kwRange) {
        return SrcIDb1Tuple {SrcIDb1Doc::DUMMY(), kwRange};
    }
    const bool isDummy() const {
        return *this == DUMMY(this->dbKwRange);
    }

    //--------------------------------------------------------------------------
    // `IDbTuple`

    using IDbTuple<SrcIDb1Doc, Kw>::IDbTuple;
    SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);

    std::string toStr() const override;
    std::string toPrintableStr() const override;
    static SrcIDb1Tuple fromStr(const std::string& str);
    static SrcIDb1Tuple fromUstr(const ustring& ustr);

    Kw getKw() const { return this->dbDoc.kw; }
    Range<IdAlias> getIdAliasRange() const { return this->dbDoc.idAliasRange; }

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};
