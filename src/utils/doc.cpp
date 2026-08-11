#include "utils/doc.h"

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
// `IDbDoc`
//==============================================================================


template <class T, class DbKw>
IDbDoc<T, DbKw>::IDbDoc(const T& val, const Range<DbKw>& dbKwRange) {
    this->val = val;
    this->dbKwRange = dbKwRange;
}


template <class T, class DbKw>
T IDbDoc<T, DbKw>::get() const {
    return this->val;
}


template <class T, class DbKw>
Range<DbKw> IDbDoc<T, DbKw>::getDbKwRange() const {
    return this->dbKwRange;
}


template <class T, class DbKw>
ustring IDbDoc<T, DbKw>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <class T, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbDoc<T, DbKw>& iDbDoc) {
    return os << iDbDoc.toStr();
}


//==============================================================================
// `Doc`
//==============================================================================


template <class DbKw>
const std::string Doc<DbKw>::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\),(-?[0-9]+--?[0-9]+)";


template <class DbKw>
const std::regex Doc<DbKw>::REGEX(Doc<DbKw>::REGEX_STR);


template <class DbKw>
Doc<DbKw>::Doc(const std::tuple<Id, Kw, Op>& val, const Range<DbKw>& dbKwRange) :
    IDbDoc<std::tuple<Id, Kw, Op>, DbKw>(val, dbKwRange) {}


template <class DbKw>
Doc<DbKw>::Doc(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    Doc<DbKw>(std::tuple {id, kw, op}, dbKwRange) {}


template <class DbKw>
std::string Doc<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->getId() << "," << this->getKw() << ","
       << static_cast<char>(this->getOp()) << "),"
       << this->dbKwRange;
    return ss.str();
}


template <class DbKw>
Doc<DbKw> Doc<DbKw>::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, Doc<DbKw>::REGEX) || matches.size() != 5) {
        std::cerr << "Error: Doc::fromUstr(): bad string \"" << str << "\" passed" << std::endl
                  << "Regex to match is \"" << Doc<DbKw>::REGEX_STR << "\"; matched groups are:"
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
    return Doc<DbKw> {id, kw, op, dbKwRange};
}


template <class DbKw>
Doc<DbKw> Doc<DbKw>::genDummy(const Range<DbKw>& dbKwRange) {
    return Doc<DbKw> {DUMMY, DUMMY, Op::DUMMY, dbKwRange};
}


template <class DbKw>
Id Doc<DbKw>::getId() const {
    return std::get<0>(this->val);
}


template <class DbKw>
Kw Doc<DbKw>::getKw() const {
    return std::get<1>(this->val);
}


template <class DbKw>
Op Doc<DbKw>::getOp() const {
    return std::get<2>(this->val);
}


template class IDbDoc<std::tuple<Id, Kw, Op>, Kw>;
//template class IDbDoc<std::tuple<Id, Kw, Op>, IdAlias>;

template class Doc<Kw>;
//template class Doc<IdAlias>;

template std::ostream& operator <<(
    std::ostream& os, const IDbDoc<std::tuple<Id, Kw, Op>, Kw>& iDbDoc
);
//template std::ostream& operator <<(
//    std::ostream& os, const IDbDoc<std::tuple<Id, Kw, Op>, IdAlias>& iDbDoc
//);


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


const std::string SrcIDb1Doc::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+--?[0-9]+)\\),(-?[0-9]+--?[0-9]+)";


const std::regex SrcIDb1Doc::REGEX(SrcIDb1Doc::REGEX_STR);


SrcIDb1Doc::SrcIDb1Doc(const std::pair<Kw, Range<IdAlias>>& val, const Range<Kw>& kwRange) :
    IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>(val, kwRange) {}


SrcIDb1Doc::SrcIDb1Doc(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    SrcIDb1Doc(std::pair {kw, idAliasRange}, kwRange) {}


std::string SrcIDb1Doc::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")," << this->dbKwRange;
    return ss.str();
}


SrcIDb1Doc SrcIDb1Doc::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, SrcIDb1Doc::REGEX) || matches.size() != 4) {
        std::cerr << "Error: SrcIDb1Doc::fromUstr(): bad string \"" << ustr << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << SrcIDb1Doc::REGEX_STR << "\"; matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }
    Kw kw = std::stol(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[3].str());
    return SrcIDb1Doc {kw, idAliasRange, kwRange};
}


SrcIDb1Doc SrcIDb1Doc::genDummy(const Range<Kw>& kwRange) {
    return SrcIDb1Doc {DUMMY, DUMMY_RANGE<IdAlias>(), kwRange};
}


template class IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>;

template std::ostream& operator <<(
    std::ostream& os, const IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw>& iDbDoc
);
