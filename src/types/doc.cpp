#include "types/doc.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <regex>
#include <string>

#include "types/basic_types.h"
#include "types/range.h"
#include "types/ustring.h"

#include "utils/debug.h"


//==============================================================================
// `IDbDoc`
//==============================================================================


ustring IDbDoc::toUstr() const {
    return ::utils::ustr::toUstr(this->toStr());
}


std::ostream& operator <<(std::ostream& os, const IDbDoc& iDbDoc) {
    return os << iDbDoc.toPrintableStr();
}


//==============================================================================
// `Doc`
//==============================================================================


const std::string Doc::REGEX_STR = "(-?[0-9]+),(-?[0-9]+)([I|D|X])";


const std::regex Doc::REGEX(REGEX_STR);


std::string Doc::toStr() const {
    return std::format("{},{}{}", this->id, this->kw, static_cast<char>(this->op));
}


std::string Doc::toPrintableStr() const {
    return std::format("({},{},{})", this->id, this->kw, static_cast<char>(this->op));
}


Doc Doc::fromStr(const std::string& str) {
    std::smatch matches;
    bool isMatchFound = std::regex_search(str, matches, REGEX);
    DEBUG_ONLY({
        if (!isMatchFound || matches.size() != 4) {
            std::cerr << "Error: Doc::fromStr(): bad string \"" << str << "\" passed\n"
                      << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                      << std::endl;
            for (auto match : matches) {
                std::cerr << match.str() << std::endl;
            }
            std::exit(EXIT_FAILURE);
        }
    });

    Id id = std::stoll(matches[1].str());
    Kw kw = std::stoll(matches[2].str());
    Op op = static_cast<Op>(matches[3].str()[0]);
    return Doc {id, kw, op};
}


Doc Doc::fromUstr(const ustring& ustr) {
    return fromStr(::utils::ustr::toStr(ustr));
}


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


const std::string SrcIDb1Doc::REGEX_STR = "(-?[0-9]+),(-?[0-9]+--?[0-9]+)";


const std::regex SrcIDb1Doc::REGEX(REGEX_STR);


std::string SrcIDb1Doc::toStr() const {
    return std::format("{},{}", this->kw, this->idAliasRange);
}


std::string SrcIDb1Doc::toPrintableStr() const {
    return std::format("({},{})", this->kw, this->idAliasRange);
}


SrcIDb1Doc SrcIDb1Doc::fromStr(const std::string& str) {
    std::smatch matches;
    bool isMatchFound = std::regex_search(str, matches, REGEX);
    DEBUG_ONLY({
        if (!isMatchFound || matches.size() != 3) {
            std::cerr << "Error: SrcIDb1Doc::fromStr(): bad string \"" << str << "\" passed\n"
                      << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                      << std::endl;
            for (auto match : matches) {
                std::cerr << match.str() << std::endl;
            }
            std::exit(EXIT_FAILURE);
        }
    });

    Kw kw = std::stoll(matches[1].str());
    Range<IdAlias> idAliasRange = Range<IdAlias>::fromStr(matches[2].str());
    return SrcIDb1Doc {kw, idAliasRange};
}


SrcIDb1Doc SrcIDb1Doc::fromUstr(const ustring& ustr) {
    return fromStr(::utils::ustr::toStr(ustr));
}
