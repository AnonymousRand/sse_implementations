#include "utils/tuple.h"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "utils/range.h"
#include "utils/types.h"
#include "utils/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


template <class DbDoc, class DbKw>
IDbTuple<DbDoc, DbKw>::IDbTuple(const DbDoc& dbDoc, const Range<DbKw>& dbKwRange) {
    this->dbDoc = dbDoc;
    this->dbKwRange = dbKwRange;
}


template <class DbDoc, class DbKw>
DbDoc IDbTuple<DbDoc, DbKw>::getDbDoc() const {
    return this->dbDoc;
}


template <class DbDoc, class DbKw>
Range<DbKw> IDbTuple<DbDoc, DbKw>::getDbKwRange() const {
    return this->dbKwRange;
}


template <class DbDoc, class DbKw>
ustring IDbTuple<DbDoc, DbKw>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <class DbDoc, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbTuple<DbDoc, DbKw>& iDbTuple) {
    return os << iDbTuple.toStr();
}


//==============================================================================
// `Tuple`
//==============================================================================


template <class DbKw>
const std::string Tuple<DbKw>::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\),(-?[0-9]+--?[0-9]+)";


template <class DbKw>
const std::regex Tuple<DbKw>::REGEX(REGEX_STR);


template <class DbKw>
Tuple<DbKw>::Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    Tuple<DbKw>(std::tuple {id, kw, op}, dbKwRange) {}


template <class DbKw>
Id Tuple<DbKw>::getId() const {
    return std::get<0>(this->dbDoc);
}


template <class DbKw>
Kw Tuple<DbKw>::getKw() const {
    return std::get<1>(this->dbDoc);
}


template <class DbKw>
Op Tuple<DbKw>::getOp() const {
    return std::get<2>(this->dbDoc);
}


template <class DbKw>
std::string Tuple<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->getId() << "," << this->getKw() << "," << static_cast<char>(this->getOp())
       << ")," << this->dbKwRange;
    return ss.str();
}


template <class DbKw>
Tuple<DbKw> Tuple<DbKw>::fromUstr(const ustring& ustr) {
    return Tuple<DbKw>::fromStr(::utils::toStr(ustr));
}


template <class DbKw>
Tuple<DbKw> Tuple<DbKw>::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, REGEX) || matches.size() != 5) {
        std::cerr << "Error: Tuple::fromUstr(): bad string \"" << str << "\" passed" << std::endl
                  << "Regex to match is \"" << REGEX_STR << "\"; " << "matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

    Id id = std::stoi(matches[1].str());
    Kw kw = std::stoi(matches[2].str());
    Op op = static_cast<Op>(matches[3].str()[0]);
    Range<DbKw> dbKwRange = Range<DbKw>::fromStr(matches[4].str());
    return Tuple<DbKw> {id, kw, op, dbKwRange};
}


template <class DbKw>
Tuple<DbKw> Tuple<DbKw>::genDummy(const Range<DbKw>& dbKwRange) {
    return Tuple<DbKw> {DUMMY, DUMMY, Op::DUMMY, dbKwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbTuple<std::tuple<Id, Kw, Op>, Kw>;
//template class IDbTuple<std::tuple<Id, Kw, Op>, IdAlias>;


template class Tuple<Kw>;
//template class Tuple<IdAlias>;


template std::ostream& operator <<(
    std::ostream& os, const IDbTuple<std::tuple<Id, Kw, Op>, Kw>& iDbTuple
);
//template std::ostream& operator <<(
//    std::ostream& os, const IDbTuple<std::tuple<Id, Kw, Op>, IdAlias>& iDbTuple
//);


//==============================================================================
// `SrcIDb1Tuple`
//==============================================================================


const std::string SrcIDb1Tuple::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+--?[0-9]+)\\),(-?[0-9]+--?[0-9]+)";


const std::regex SrcIDb1Tuple::REGEX(REGEX_STR);


SrcIDb1Tuple::SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    SrcIDb1Tuple(std::pair {kw, idAliasRange}, kwRange) {}


Kw SrcIDb1Tuple::getKw() const {
    return this->dbDoc.first;
}


Range<IdAlias> SrcIDb1Tuple::getIdAliasRange() const {
    return this->dbDoc.second;
}


std::string SrcIDb1Tuple::toStr() const {
    std::stringstream ss;
    ss << "(" << this->dbDoc.first << "," << this->dbDoc.second << ")," << this->dbKwRange;
    return ss.str();
}


SrcIDb1Tuple ScrIDb1Tuple::fromUstr(const ustring& ustr) {
    return SrcIDb1Tuple::fromStr(::utils::toStr(ustr));
}


SrcIDb1Tuple SrcIDb1Tuple::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, REGEX) || matches.size() != 4) {
        std::cerr << "Error: SrcIDb1Tuple::fromUstr(): bad string \"" << ustr << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

    Kw kw = std::stol(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[3].str());
    return SrcIDb1Tuple {kw, idAliasRange, kwRange};
}


SrcIDb1Tuple SrcIDb1Tuple::genDummy(const Range<Kw>& kwRange) {
    return SrcIDb1Tuple {DUMMY, DUMMY_RANGE<IdAlias>(), kwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw>;


template std::ostream& operator <<(
    std::ostream& os, const IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw>& iDbTuple
);
