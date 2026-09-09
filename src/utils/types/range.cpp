#include "utils/types/range.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <regex>
#include <string>

#include "utils/debug.h"
#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


template <std::integral T>
const std::string Range<T>::REGEX_STR = "(-?[0-9]+)-(-?[0-9]+)";

template <std::integral T>
const std::regex Range<T>::REGEX(REGEX_STR);


template <std::integral T>
T Range<T>::size() const {
    // (`+ 1` as both ends are inclusive)
    return this->end - this->start + 1;
}


template <std::integral T>
bool Range<T>::contains(const Range<T>& target) const {
    return this->start <= target.start && this->end >= target.end;
}


template <std::integral T>
bool Range<T>::contains(T target) const {
    return this->start <= target && this->end >= target;
}


template <std::integral T>
bool Range<T>::isDisjointFrom(const Range<T>& target) const {
    return this->end < target.start || this->start > target.end;
}


template <std::integral T>
std::string Range<T>::toStr() const {
    return std::format("{}-{}", this->start, this->end);
}


template <std::integral T>
ustring Range<T>::toUstr() const {
    return ::utils::ustr::toUstr(this->toStr());
}


template <std::integral T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::smatch matches;
    bool isMatchFound = std::regex_search(str, matches, REGEX);
    DEBUG_ONLY({
        if (!isMatchFound || matches.size() != 3) {
            std::cerr << "Error: Range::fromStr(): bad string \"" << str << "\" passed\n"
                      << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:"
                      << std::endl;
            for (auto match : matches) {
                std::cerr << match.str() << std::endl;
            }
            std::exit(EXIT_FAILURE);
        }
    });

    return Range<T> {
        T(std::stoll(matches[1].str())),
        T(std::stoll(matches[2].str()))
    };
}


template <std::integral T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toStr();
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Range<Kw>;
//template class Range<IdAlias>;


template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
//template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);
