#include "utils/range.h"

#include <concepts>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

#include "utils/types.h"
#include "utils/ustring.h"


template <std::integral T>
const std::string Range<T>::REGEX_STR = "(-?[0-9]+)-(-?[0-9]+)";


template <std::integral T>
const std::regex Range<T>::REGEX(REGEX_STR);


template <std::integral T>
Range<T>::Range(T start, T end) : std::pair<T, T> {start, end} {}


template <std::integral T>
T Range<T>::size() const {
    // (`+ 1` as both ends are inclusive)
    return this->second - this->first + 1;
}


template <std::integral T>
bool Range<T>::contains(const Range<T>& target) const {
    return this->first <= target.first && this->second >= target.second;
}


template <std::integral T>
bool Range<T>::contains(T target) const {
    return this->first <= target && this->second >= target;
}


template <std::integral T>
bool Range<T>::isDisjointFrom(const Range<T>& target) const {
    return this->second < target.first || this->first > target.second;
}


template <std::integral T>
std::string Range<T>::toStr() const {
    std::stringstream ss;
    ss << this->first << "-" << this->second;
    return ss.str();
}


template <std::integral T>
ustring Range<T>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <std::integral T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, REGEX) || matches.size() != 3) {
        std::cerr << "Error: Range::fromStr(): bad string \"" << str << "\" passed" << std::endl
                  << "Regex to match is \"" << REGEX_STR << "\"; matched groups are:" << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

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
