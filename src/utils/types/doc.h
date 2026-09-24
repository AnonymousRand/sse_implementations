#pragma once

#include <concepts>
#include <iostream>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


//==============================================================================
// `IDbDoc`
//==============================================================================


/**
 * interface for the "document" part of database tuples.
 */
struct IDbDoc {
public:
    virtual void encode(uchar* ret) const = 0;
    virtual std::string toPrettyStr() const = 0;

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

    // note: these cannot be member *variables* if we wish to initialize them here in the class
    // body, as that requires the class itself (i.e. variable type) to be fully initialized first
    static Doc DUMMY() { return Doc {::DUMMY, ::DUMMY, Op::DUMMY}; }
    bool isDummy() const { return *this == DUMMY(); }

    // note: can't use aggregate initialization here as that only works if the class
    // has no virtual methods
    Doc(Id id, Kw kw, Op op) : id(id), kw(kw), op(op) {}

    void encode(uchar* ret) const override;
    static Doc decode(const uchar* encoding);
    std::string toPrettyStr() const override;
    static const int ENCODING_LEN;

    // need to explicitly declare this again since we have additional member variables in this child
    friend bool operator ==(const Doc& doc1, const Doc& doc2) = default;
};


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


struct SrcIDb1Doc : public IDbDoc {
public:
    friend struct SrcIDb1Tuple;

    Kw kw;
    Range<IdAlias> idAliasRange;

    static SrcIDb1Doc DUMMY() { return SrcIDb1Doc {::DUMMY, Range<IdAlias>::DUMMY()}; }
    bool isDummy() const { return *this == DUMMY(); }

    SrcIDb1Doc(Kw kw, Range<IdAlias> idAliasRange) : kw(kw), idAliasRange(idAliasRange) {}

    void encode(uchar* ret) const override;
    static SrcIDb1Doc decode(const uchar* encoding);
    std::string toPrettyStr() const override;
    static const int ENCODING_LEN;

    friend bool operator ==(const SrcIDb1Doc& doc1, const SrcIDb1Doc& doc2) = default;
};
