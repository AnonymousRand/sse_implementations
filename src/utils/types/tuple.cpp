#include "utils/types/tuple.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


template <IsDbDoc DbDoc, class DbKw>
ustring IDbTuple<DbDoc, DbKw>::encode() const {
    ustring ret = this->dbDoc.encode();
    ret += this->dbKwRange.encode();
    return ret;
}


template <IsDbDoc DbDoc, class DbKw>
void IDbTuple<DbDoc, DbKw>::decode(const ustring& encoding, IDbTuple<DbDoc, DbKw>& ret) {
    ret.dbDoc = DbDoc::decode(encoding);
    ret.dbKwRange = Range<DbKw>::decode(encoding, DbDoc::ENCOD_LEN);
}


template <IsDbDoc DbDoc, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbTuple<DbDoc, DbKw>& iDbTuple) {
    return os << iDbTuple.toPrettyStr();
}


//==============================================================================
// `Tuple`
//==============================================================================


template <class DbKw>
Tuple<DbKw>::Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    Tuple<DbKw> {Doc {id, kw, op}, dbKwRange} {}


template <class DbKw>
std::string Tuple<DbKw>::toPrettyStr() const {
    return this->dbDoc.toPrettyStr() + std::format(",{}", this->dbKwRange);
}


//------------------------------------------------------------------------------
// explicit template instantiations for `IDbTuple`


template class IDbTuple<Doc, Kw>;
//template class IDbTuple<Doc, IdAlias>;


template class Tuple<Kw>;
//template class Tuple<IdAlias>;


template std::ostream& operator <<(std::ostream& os, const IDbTuple<Doc, Kw>& iDbTuple);
//template std::ostream& operator <<(std::ostream& os, const IDbTuple<Doc, IdAlias>& iDbTuple);


//==============================================================================
// `SrcIDb1Tuple`
//==============================================================================


SrcIDb1Tuple::SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    SrcIDb1Tuple {SrcIDb1Doc {kw, idAliasRange}, kwRange} {}


std::string SrcIDb1Tuple::toPrettyStr() const {
    return this->dbDoc.toPrettyStr() + std::format(",{}", this->dbKwRange);
}


//------------------------------------------------------------------------------
// explicit template instantiations for `IDbTuple`


template class IDbTuple<SrcIDb1Doc, Kw>;


template std::ostream& operator <<(std::ostream& os, const IDbTuple<SrcIDb1Doc, Kw>& iDbTuple);
