#include "utils/types/doc.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "config.h"

#include "utils/misc.h"
#include "utils/types/basic_types.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


//==============================================================================
// `IDbDoc`
//==============================================================================


std::ostream& operator <<(std::ostream& os, const IDbDoc& iDbDoc) {
    return os << iDbDoc.toPrettyStr();
}


//==============================================================================
// `Doc`
//==============================================================================


ustring Doc::encode() const {
    ustring ret = utils::misc::encodeBigint(this->id, config::INT_MAX_BYTES);
    ret += utils::misc::encodeBigint(this->kw, config::INT_MAX_BYTES);
    ret += static_cast<char>(this->op);
    return ret;
}


int Doc::ENCODING_LEN = 2 * config::INT_MAX_BYTES + 1;


Doc Doc::decode(const ustring& encoding) {
    Id id = utils::misc::decodeBigint(encoding, 0, config::INT_MAX_BYTES);
    Kw kw = utils::misc::decodeBigint(encoding, config::INT_MAX_BYTES, config::INT_MAX_BYTES);
    Op op = static_cast<Op>(encoding[2 * config::INT_MAX_BYTES]);
    return Doc {id, kw, op};
}


std::string Doc::toPrettyStr() const {
    return std::format("({},{},{})", this->id, this->kw, static_cast<char>(this->op));
}


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


ustring SrcIDb1Doc::encode() const {
    ustring ret = utils::misc::encodeBigint(this->kw, config::INT_MAX_BYTES);
    ret += this->idAliasRange.encode();
    return ret;
}


int SrcIDb1Doc::ENCODING_LEN = config::INT_MAX_BYTES + Range<IdAlias>::ENCODING_LEN;


SrcIDb1Doc SrcIDb1Doc::decode(const ustring& encoding) {
    Kw kw = utils::misc::decodeBigint(encoding, 0, config::INT_MAX_BYTES);
    Range<IdAlias> idAliasRange = Range<IdAlias>::decode(encoding, config::INT_MAX_BYTES);
    return SrcIDb1Doc {kw, idAliasRange};
}


std::string SrcIDb1Doc::toPrettyStr() const {
    return std::format("({},{})", this->kw, this->idAliasRange);
}
