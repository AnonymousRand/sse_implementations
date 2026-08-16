#include "utils/types/tuple.h"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


//==============================================================================
// `IDbTuple`
//==============================================================================


template <class DbDoc, class DbKw>
IDbTuple<DbDoc, DbKw>::IDbTuple(const DbDoc& dbDoc, const Range<DbKw>& dbKwRange) {
    this->dbDoc = dbDoc;
    this->dbKwRange = dbKwRange;
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

    Id id = std::stoll(matches[1].str());
    Kw kw = std::stoll(matches[2].str());
    Op op = static_cast<Op>(matches[3].str()[0]);
    Range<DbKw> dbKwRange = Range<DbKw>::fromStr(matches[4].str());
    return Tuple<DbKw> {id, kw, op, dbKwRange};
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


SrcIDb1Tuple SrcIDb1Tuple::fromUstr(const ustring& ustr) {
    return SrcIDb1Tuple::fromStr(::utils::toStr(ustr));
}


SrcIDb1Tuple SrcIDb1Tuple::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, REGEX) || matches.size() != 4) {
        std::cerr << "Error: SrcIDb1Tuple::fromUstr(): bad string \"" << str << "\" passed"
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


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw>;


template std::ostream& operator <<(
    std::ostream& os, const IDbTuple<std::pair<Kw, Range<IdAlias>>, Kw>& iDbTuple
);


//==============================================================================
// utils
//==============================================================================


namespace utils {


// (we need the general case of this function to be able to call it from within the general context
// of `IStaticPointSse`; it just does nothing except in the template specialization below)
template <IsDbTuple DbTuple>
std::vector<DbTuple> cleanUpResults(const std::vector<DbTuple>& dbTuples) {
    return dbTuples;
}


// template specialize this method for just `Tuple<>` instead of all
// SSE classes that use it
template <>
std::vector<Tuple<>> cleanUpResults(const std::vector<Tuple<>>& tuples) {
    std::vector<Tuple<>> newTuples;
    std::unordered_set<Id> deletedIds;

    // find all cancellation tuples
    for (const Tuple<>& tuple : tuples) {
        Op op = tuple.getOp();
        if (op == Op::DEL) {
            Id id = tuple.getId();
            deletedIds.insert(id);
        }
    }
    // copy over vector without deleted (or dummy) tuples, as well as no dummy ids
    for (const Tuple<>& tuple : tuples) {
        Id id = tuple.getId();
        Op op = tuple.getOp();
        if (id != DUMMY && op == Op::INS && deletedIds.count(id) == 0) {
            newTuples.push_back(tuple);
        }
    }

    return newTuples;
}


//------------------------------------------------------------------------------
// explicit template instantiations


// remaining explicit template specializations beyond the one earlier
template std::vector<SrcIDb1Tuple> cleanUpResults(const std::vector<SrcIDb1Tuple>& tuples);
//template std::vector<Tuple<IdAlias>> cleanUpResults(const std::vector<Tuple<IdAlias>>& tuples);


} // namespace `utils`
