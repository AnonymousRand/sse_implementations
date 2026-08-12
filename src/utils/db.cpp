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
// `IDbRecord`
//==============================================================================


template <class T, class DbKw>
IDbRecord<T, DbKw>::IDbRecord(const T& val, const Range<DbKw>& dbKwRange) {
    this->val = val;
    this->dbKwRange = dbKwRange;
}


template <class T, class DbKw>
T IDbRecord<T, DbKw>::get() const {
    return this->val;
}


template <class T, class DbKw>
Range<DbKw> IDbRecord<T, DbKw>::getDbKwRange() const {
    return this->dbKwRange;
}


template <class T, class DbKw>
ustring IDbRecord<T, DbKw>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <class T, class DbKw>
std::ostream& operator <<(std::ostream& os, const IDbRecord<T, DbKw>& iDbRecord) {
    return os << iDbRecord.toStr();
}


//==============================================================================
// `Record`
//==============================================================================


template <class DbKw>
const std::string Record<DbKw>::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+),([I|D|-])\\),(-?[0-9]+--?[0-9]+)";


template <class DbKw>
const std::regex Record<DbKw>::REGEX(Record<DbKw>::REGEX_STR);


template <class DbKw>
DbRecord<DbKw>::Record(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange) :
    IDbRecord<>(std::tuple {id, kw, op}, dbKwRange) {}


template <class DbKw>
Id Record<DbKw>::getId() const {
    return std::get<0>(this->val);
}


template <class DbKw>
Kw Record<DbKw>::getKw() const {
    return std::get<1>(this->val);
}


template <class DbKw>
Op Record<DbKw>::getOp() const {
    return std::get<2>(this->val);
}


template <class DbKw>
std::string Record<DbKw>::toStr() const {
    std::stringstream ss;
    ss << "(" << this->getId() << "," << this->getKw() << "," << static_cast<char>(this->getOp())
       << ")," << this->dbKwRange;
    return ss.str();
}


template <class DbKw>
DbRecord<DbKw> Record<DbKw>::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, Record<DbKw>::REGEX) || matches.size() != 5) {
        std::cerr << "Error: Record::fromUstr(): bad string \"" << str << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << Record<DbKw>::REGEX_STR << "\"; "
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
    return Record<DbKw> {id, kw, op, dbKwRange};
}


template <class DbKw>
DbRecord<DbKw> Record<DbKw>::genDummy(const Range<DbKw>& dbKwRange) {
    return Record<DbKw> {DUMMY, DUMMY, Op::DUMMY, dbKwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbRecord<std::tuple<Id, Kw, Op>, Kw>;
//template class IDbRecord<std::tuple<Id, Kw, Op>, IdAlias>;

template class Record<Kw>;
//template class Record<IdAlias>;

template std::ostream& operator <<(
    std::ostream& os, const IDbRecord<std::tuple<Id, Kw, Op>, Kw>& iDbRecord
);
//template std::ostream& operator <<(
//    std::ostream& os, const IDbRecord<std::tuple<Id, Kw, Op>, IdAlias>& iDbRecord
//);


//==============================================================================
// `SrcIDb1Record`
//==============================================================================


const std::string SrcIDb1Record::REGEX_STR =
    "\\((-?[0-9]+),(-?[0-9]+--?[0-9]+)\\),(-?[0-9]+--?[0-9]+)";


const std::regex SrcIDb1Record::REGEX(SrcIDb1Record::REGEX_STR);


SrcIDb1Record::SrcIDb1Record(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange) :
    IDbRecord<>(std::pair {kw, idAliasRange}, kwRange) {}


SrcIDb1Record::SrcIDb1Record(const SrcIDb1Record& srcIDb1Record) :
    val(srcIDb1Record.get()), dbKwRange(srcIDb1Record.getDbKwRange()) {}


Kw SrcIDb1Record::getKw() const {
    return this->val.first;
}


Range<IdAlias> SrcIDb1Record::getIdAliasRange() const {
    return this->val.second;
}


std::string SrcIDb1Record::toStr() const {
    std::stringstream ss;
    ss << "(" << this->val.first << "," << this->val.second << ")," << this->dbKwRange;
    return ss.str();
}


SrcIDb1Record SrcIDb1Record::fromUstr(const ustring& ustr) {
    std::string str = ::utils::toStr(ustr);
    std::smatch matches;
    if (!std::regex_search(str, matches, SrcIDb1Record::REGEX) || matches.size() != 4) {
        std::cerr << "Error: SrcIDb1Record::fromUstr(): bad string \"" << ustr << "\" passed"
                  << std::endl
                  << "Regex to match is \"" << SrcIDb1Record::REGEX_STR << "\"; matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

    Kw kw = std::stol(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    Range<Kw> kwRange = Range<Kw>::fromStr(matches[3].str());
    return SrcIDb1Record {kw, idAliasRange, kwRange};
}


SrcIDb1Record SrcIDb1Record::genDummy(const Range<Kw>& kwRange) {
    return SrcIDb1Record {DUMMY, DUMMY_RANGE<IdAlias>(), kwRange};
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class IDbRecord<std::pair<Kw, Range<IdAlias>>, Kw>;

template std::ostream& operator <<(
    std::ostream& os, const IDbRecord<std::pair<Kw, Range<IdAlias>>, Kw>& iDbRecord
);
