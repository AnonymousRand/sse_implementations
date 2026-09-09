#pragma once

#include <concepts>
#include <iostream>
#include <regex>
#include <string>

#include "types/basic_types.h"
#include "types/range.h"
#include "types/ustring.h"


//==============================================================================
// `IDbDoc`
//==============================================================================


/**
 * interface for the "document" part of database tuples.
 */
struct IDbDoc {
public:
    // `toStr()` should be the most compact possible unambiguous encoding, for efficient storage
    // whereas `toPrintableStr()` can be prettier :3
    virtual std::string toStr() const = 0;
    virtual std::string toPrintableStr() const = 0;
    ustring toUstr() const;

    friend bool operator ==(const IDbDoc& iDbDoc1, const IDbDoc& iDbDoc2) = default;
    friend std::ostream& operator <<(std::ostream& os, const IDbDoc& iDbDoc);
};


template <class T>
concept IsDbDoc = std::derived_from<T, IDbDoc>;


//==============================================================================
// `Doc`
//==============================================================================


struct Doc : public IDbDoc {
public:
    // this allows `Tuple` to access `Doc`'s regex stuff without `Doc` needing to expose it publicly
    template <class DbKw>
    friend struct Tuple;

    Id id;
    Kw kw;
    Op op;

    // note: can't use aggregate initialization here as that only works if the class
    // has no virtual methods
    Doc(Id id, Kw kw, Op op) : id(id), kw(kw), op(op) {}

    // note: these cannot be member *variables* if we wish to initialize them here in the class
    // body, as that requires the class itself (i.e. variable type) to be fully initialized first
    static const Doc DUMMY() {
        return Doc {::DUMMY, ::DUMMY, Op::DUMMY};
    }
    const bool isDummy() const {
        return *this == DUMMY();
    }

    //--------------------------------------------------------------------------
    // `IDbDoc`

    std::string toStr() const override;
    std::string toPrintableStr() const override;
    static Doc fromRegexMatches(const std::smatch& matches);

    // need to explicitly declare this again since we have additional member variables in this child
    friend bool operator ==(const Doc& doc1, const Doc& doc2) = default;

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
    static const int REGEX_SUBMATCH_COUNT;
};


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


struct SrcIDb1Doc : public IDbDoc {
public:
    friend struct SrcIDb1Tuple;

    Kw kw;
    Range<IdAlias> idAliasRange;

    SrcIDb1Doc(Kw kw, Range<IdAlias> idAliasRange) : kw(kw), idAliasRange(idAliasRange) {}

    static const SrcIDb1Doc DUMMY() {
        return SrcIDb1Doc {::DUMMY, Range<IdAlias>::DUMMY()};
    }
    const bool isDummy() const {
        return *this == DUMMY();
    }

    //--------------------------------------------------------------------------
    // `IDbDoc`

    std::string toStr() const override;
    std::string toPrintableStr() const override;
    static SrcIDb1Doc fromRegexMatches(const std::smatch& matches);

    friend bool operator ==(const SrcIDb1Doc& doc1, const SrcIDb1Doc& doc2) = default;

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
    static const int REGEX_SUBMATCH_COUNT;
};
