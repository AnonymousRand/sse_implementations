#pragma once

#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <tuple>
#include <utility>

#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


//==============================================================================
// `IDbDoc`
//==============================================================================


// interface for documents in dataset; also store the `DbKw` range (e.g. Log-SRC replications) they're stored with
// in their respective datasets, so that we can easily fetch them in plaintext for things like SDa
// (otherwise they might be only accessible via the encrypted "label" in the encrypted index, which can be a hash/PRF
// and hence not easily reversible, unlike `DbDoc`s which are just encrypted and can be easily decrypted)
template <class T, class DbKw>
class IDbDoc {
public:
    IDbDoc() = default;
    IDbDoc(const T& val, const Range<DbKw>& dbKwRange);

    T get() const;
    Range<DbKw> getDbKwRange() const;
    virtual std::string toStr() const = 0;
    ustring toUstr() const;

    template <class T2, class DbKw2>
    friend std::ostream& operator <<(std::ostream& os, const IDbDoc<T2, DbKw2>& iDbDoc);

protected:
    T val;
    Range<DbKw> dbKwRange;
};


//==============================================================================
// `Doc`
//==============================================================================


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw>
class Doc : public IDbDoc<std::tuple<Id, Kw, Op>, DbKw> {
public:
    Doc() = default;
    Doc(const std::tuple<Id, Kw, Op>& val, const Range<DbKw>& dbKwRange);
    Doc(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

    std::string toStr() const override;
    static Doc<DbKw> fromUstr(const ustring& ustr);
    static Doc<DbKw> genDummy(const Range<DbKw>& dbKwRange);

    Id getId() const;
    Kw getKw() const;
    Op getOp() const;

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


template <class DbKw>
struct std::hash<Doc<DbKw>> {
    inline std::size_t operator ()(const Doc<DbKw>& doc) const noexcept {
        return std::hash<std::string>{}(doc.toStr());
    }
};


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


class SrcIDb1Doc : public IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw> {
public:
    SrcIDb1Doc() = default;
    SrcIDb1Doc(const std::pair<Kw, Range<IdAlias>>& val, const Range<Kw>& kwRange);
    SrcIDb1Doc(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);

    std::string toStr() const override;
    static SrcIDb1Doc fromUstr(const ustring& ustr);
    static SrcIDb1Doc genDummy(const Range<Kw>& kwRange);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};
