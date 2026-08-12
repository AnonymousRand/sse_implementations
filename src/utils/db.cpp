#include "utils/db.h"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


//==============================================================================
// `IDbEntry`
//==============================================================================


template <class T, class DbKw>
IDbEntry<T, DbKw>::IDbEntry(const T& val, const Range<DbKw>& dbKwRange) {
    this->val = val;
    this->dbKwRange = dbKwRange;
}


template <class T, class DbKw>
T IDbEntry<T, DbKw>::get() const {
    return this->val;
}


template <class T, class DbKw>
Range<DbKw> IDbEntry<T, DbKw>::getDbKwRange() const {
    return this->dbKwRange;
}


template <class T, class DbKw>
ustring IDbEntry<T, DbKw>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <class T, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbEntry<T, DbKw>& iDbEntry) {
    return os << iDbEntry.toStr();
}


//==============================================================================
// `DefaultDbEntry`
//==============================================================================


template <class DbKw>
const std::string DefaultDbEntry<DbKw>::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\),(-?[0-9]+--?[0-9]+)";


template <class DbKw>
const std::regex DefaultDbEntry<DbKw>::REGEX(DefaultDbEntry<DbKw>::REGEX_STR);


template <class DbKw>
DbEntry<DbKw>::DefaultDbEntry(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    IDbEntry<>(std::tuple {id, kw, op}, dbKwRange) {}


template <class DbKw>
Id DefaultDbEntry<DbKw>::getId() const {
    return std::get<0>(this->val);
}


template <class DbKw>
Kw DefaultDbEntry<DbKw>::getKw() const {
    return std::get<1>(this->val);
}


template <class DbKw>
Op DefaultDbEntry<DbKw>::getOp() const {
    return std::get<2>(this->val);
}


template <class DbKw>
std::string DefaultDbEntry<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->getId() << "," << this->getKw() << "," << static_cast<char>(this->getOp())
       << ")," << this->dbKwRange;
    return ss.str();
}


template <class DbKw>
DbEntry<DbKw> DefaultDbEntry<DbKw>::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, DefaultDbEntry<DbKw>::REGEX) || matches.size() != 5) {
        std::cerr << "Error: DefaultDbEntry::fromUstr(): bad string \"" << str << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << DefaultDbEntry<DbKw>::REGEX_STR << "\"; "
                  << "matched groups are:"
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
    return DefaultDbEntry<DbKw> {id, kw, op, dbKwRange};
}


template <class DbKw>
DbEntry<DbKw> DefaultDbEntry<DbKw>::genDummy(const Range<DbKw>& dbKwRange) {
    return DefaultDbEntry<DbKw> {DUMMY, DUMMY, Op::DUMMY, dbKwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbEntry<std::tuple<Id, Kw, Op>, Kw>;
//template class IDbEntry<std::tuple<Id, Kw, Op>, IdAlias>;

template class DefaultDbEntry<Kw>;
//template class DefaultDbEntry<IdAlias>;

template std::ostream& operator <<(
    std::ostream& os, const IDbEntry<std::tuple<Id, Kw, Op>, Kw>& iDbEntry
);
//template std::ostream& operator <<(
//    std::ostream& os, const IDbEntry<std::tuple<Id, Kw, Op>, IdAlias>& iDbEntry
//);


//==============================================================================
// `SrcIDb1Entry`
//==============================================================================


const std::string SrcIDb1Entry::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+--?[0-9]+)\\),(-?[0-9]+--?[0-9]+)";


const std::regex SrcIDb1Entry::REGEX(SrcIDb1Entry::REGEX_STR);


SrcIDb1Entry::SrcIDb1Entry(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    IDbEntry<>(std::pair {kw, idAliasRange}, kwRange) {}


SrcIDb1Entry::SrcIDb1Entry(const SrcIDb1Entry& srcIDb1Entry) :
    val(srcIDb1Entry.get()), dbKwRange(srcIDb1Entry.getDbKwRange()) {}


Kw SrcIDb1Entry::getKw() const {
    return this->val.first;
}


Range<IdAlias> SrcIDb1Entry::getIdAliasRange() const {
    return this->val.second;
}


std::string SrcIDb1Entry::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")," << this->dbKwRange;
    return ss.str();
}


SrcIDb1Entry SrcIDb1Entry::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, SrcIDb1Entry::REGEX) || matches.size() != 4) {
        std::cerr << "Error: SrcIDb1Entry::fromUstr(): bad string \"" << ustr << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << SrcIDb1Entry::REGEX_STR << "\"; matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

    Kw kw = std::stol(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[3].str());
    return SrcIDb1Entry {kw, idAliasRange, kwRange};
}


SrcIDb1Entry SrcIDb1Entry::genDummy(const Range<Kw>& kwRange) {
    return SrcIDb1Entry {DUMMY, DUMMY_RANGE<IdAlias>(), kwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbEntry<std::pair<Kw, Range<IdAlias>>, Kw>;

template std::ostream& operator <<(
    std::ostream& os, const IDbEntry<std::pair<Kw, Range<IdAlias>>, Kw>& iDbEntry
);
