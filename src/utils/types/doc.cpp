#include "utils/types/doc.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "config.h"

#include "utils/debug.h"
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


std::string Doc::toPrettyStr() const {
    return std::format("({},{},{})", this->id, this->kw, static_cast<char>(this->op));
}


void Doc::encode(uchar* ret) const {
    utils::misc::encodeBigint(ret, this->id, config::INT_MAX_BYTES);
    utils::misc::encodeBigint(ret + config::INT_MAX_BYTES, this->kw, config::INT_MAX_BYTES);
    ret[2 * config::INT_MAX_BYTES + 1] = static_cast<char>(this->op);
}


const int Doc::ENCODING_LEN = 2 * config::INT_MAX_BYTES + 1;


Doc Doc::decode(const uchar* encoding) {
    Id id = utils::misc::decodeBigint(encoding, config::INT_MAX_BYTES);
    Kw kw = utils::misc::decodeBigint(encoding + config::INT_MAX_BYTES, config::INT_MAX_BYTES);
    Op op = static_cast<Op>(encoding[2 * config::INT_MAX_BYTES + 1]);
    return Doc {id, kw, op};
}


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


void SrcIDb1Doc::encode(uchar* ret) const {
    utils::misc::encodeBigint(ret, this->kw, config::INT_MAX_BYTES);
    this->idAliasRange.encode(ret + config::INT_MAX_BYTES);
}


const int SrcIDb1Doc::ENCODING_LEN = config::INT_MAX_BYTES + Range<IdAlias>::ENCODING_LEN;


SrcIDb1Doc SrcIDb1Doc::decode(const uchar* encoding) {
    Kw kw = utils::misc::decodeBigint(encoding, config::INT_MAX_BYTES);
    Range<IdAlias> idAliasRange = Range<IdAlias>::decode(encoding + config::INT_MAX_BYTES);
    return SrcIDb1Doc {kw, idAliasRange};
}


std::string SrcIDb1Doc::toPrettyStr() const {
    return std::format("({},{})", this->kw, this->idAliasRange);
}
