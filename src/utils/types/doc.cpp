#include "utils/types/doc.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <regex>
#include <string>

#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/range.h"
#include "utils/types/ustring.h"


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

const int Doc::REGEX_SUBMATCH_COUNT = 3;


std::string Doc::toStr() const {
    return std::format("{},{}{}", this->id, this->kw, static_cast<char>(this->op));
}


std::string Doc::toPrintableStr() const {
    return std::format("({},{},{})", this->id, this->kw, static_cast<char>(this->op));
}


Doc Doc::fromRegexMatches(const std::smatch& matches) {
    DEBUG_ONLY({
        if (matches.size() < REGEX_SUBMATCH_COUNT + 1) {
            std::cerr << "Error: Doc::fromRegexMatches(): bad string passed\n"
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


//==============================================================================
// `SrcIDb1Doc`
//==============================================================================


const std::string SrcIDb1Doc::REGEX_STR = "(-?[0-9]+),(-?[0-9]+--?[0-9]+)";

const std::regex SrcIDb1Doc::REGEX(REGEX_STR);

const int SrcIDb1Doc::REGEX_SUBMATCH_COUNT = 2;


std::string SrcIDb1Doc::toStr() const {
    return std::format("{},{}", this->kw, this->idAliasRange);
}


std::string SrcIDb1Doc::toPrintableStr() const {
    return std::format("({},{})", this->kw, this->idAliasRange);
}


SrcIDb1Doc SrcIDb1Doc::fromRegexMatches(const std::smatch& matches) {
    DEBUG_ONLY({
        if (matches.size() < REGEX_SUBMATCH_COUNT + 1) {
            std::cerr << "Error: SrcIDb1Doc::fromRegexMatches(): bad string passed\n"
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
