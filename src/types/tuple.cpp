#include "types/tuple.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <regex>
#include <string>

#include "types/basic_types.h"
#include "types/doc.h"
#include "types/range.h"
#include "types/ustring.h"

#include "utils/debug.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


template <IsDbDoc DbDoc, class DbKw>
ustring IDbTuple<DbDoc, DbKw>::toUstr() const {
    return ::utils::ustr::toUstr(this->toStr());
}


template <IsDbDoc DbDoc, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbTuple<DbDoc, DbKw>& iDbTuple) {
    return os << iDbTuple.toPrintableStr();
}


//==============================================================================
// `Tuple`
//==============================================================================


template <class DbKw>
const std::string Tuple<DbKw>::REGEX_STR = Doc::REGEX_STR + "(-?[0-9]+--?[0-9]+)";

template <class DbKw>
const std::regex Tuple<DbKw>::REGEX(REGEX_STR);


template <class DbKw>
Tuple<DbKw>::Tuple(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    Tuple<DbKw>(Doc {id, kw, op}, dbKwRange) {}


template <class DbKw>
std::string Tuple<DbKw>::toStr() const {
    // IMPORTANT: `Op` encodings cannot be numerical for this encoding to be unambiguous for regex!
    // this is a work of art
    return this->dbDoc.toStr() + std::format("{}", this->dbKwRange);
}


template <class DbKw>
std::string Tuple<DbKw>::toPrintableStr() const {
    return this->dbDoc.toPrintableStr() + std::format(",{}", this->dbKwRange);
}


template <class DbKw>
Tuple<DbKw> Tuple<DbKw>::fromStr(const std::string& str) {
    std::smatch matches;
    bool isMatchFound = std::regex_search(str, matches, REGEX);
    DEBUG_ONLY({
        if (!isMatchFound || matches.size() != Doc::REGEX_SUBMATCH_COUNT + 2) {
            std::cerr << "Error: Tuple::fromStr(): bad string \"" << str << "\" passed\n"
                      << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                      << std::endl;
            for (auto match : matches) {
                std::cerr << match.str() << std::endl;
            }
            std::exit(EXIT_FAILURE);
        }
    });

    Doc doc = Doc::fromRegexMatches(matches);
    Range<DbKw> dbKwRange = Range<DbKw>::fromStr(matches[Doc::REGEX_SUBMATCH_COUNT + 1].str());
    return Tuple<DbKw> {doc, dbKwRange};
}


template <class DbKw>
Tuple<DbKw> Tuple<DbKw>::fromUstr(const ustring& ustr) {
    return fromStr(::utils::ustr::toStr(ustr));
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


const std::string SrcIDb1Tuple::REGEX_STR = SrcIDb1Doc::REGEX_STR + ",(-?[0-9]+--?[0-9]+)";

const std::regex SrcIDb1Tuple::REGEX(REGEX_STR);


SrcIDb1Tuple::SrcIDb1Tuple(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    SrcIDb1Tuple(SrcIDb1Doc {kw, idAliasRange}, kwRange) {}


std::string SrcIDb1Tuple::toStr() const {
    return this->dbDoc.toStr() + std::format(",{}", this->dbKwRange);
}


std::string SrcIDb1Tuple::toPrintableStr() const {
    return this->dbDoc.toPrintableStr() + std::format(",{}", this->dbKwRange);
}


SrcIDb1Tuple SrcIDb1Tuple::fromStr(const std::string& str) {
    std::smatch matches;
    bool isMatchFound = std::regex_search(str, matches, REGEX);
    DEBUG_ONLY({
        if (!isMatchFound || matches.size() != SrcIDb1Doc::REGEX_SUBMATCH_COUNT + 2) {
            std::cerr << "Error: SrcIDb1Tuple::fromStr(): bad string \"" << str << "\" passed\n"
                      << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                      << std::endl;
            for (auto match : matches) {
                std::cerr << match.str() << std::endl;
            }
            std::exit(EXIT_FAILURE);
        }
    });

    SrcIDb1Doc doc = SrcIDb1Doc::fromRegexMatches(matches);
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[SrcIDb1Doc::REGEX_SUBMATCH_COUNT + 1].str());
    return SrcIDb1Tuple {doc, kwRange};
}


SrcIDb1Tuple SrcIDb1Tuple::fromUstr(const ustring& ustr) {
    return fromStr(::utils::ustr::toStr(ustr));
}


//------------------------------------------------------------------------------
// explicit template instantiations for `IDbTuple`


template class IDbTuple<SrcIDb1Doc, Kw>;


template std::ostream& operator <<(std::ostream& os, const IDbTuple<SrcIDb1Doc, Kw>& iDbTuple);
