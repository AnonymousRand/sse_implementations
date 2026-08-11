#include "utils/range.h"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

#include "utils/sse_utils.h"
#include "utils/ustring.h"


template <class T>
const std::string Range<T>::REGEX_STR = "(-?[0-9]+)-(-?[0-9]+)";


template <class T>
const std::regex Range<T>::REGEX(Range<T>::REGEX_STR);


template <class T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T> {start, end} {}


template <class T>
T Range<T>::size() const {
    return this->second - this->first + 1;
}


template <class T>
bool Range<T>::contains(const Range<T>& target) const {
    return this->first <= target.first && this->second >= target.second;
}


template <class T>
bool Range<T>::contains(T target) const {
    return this->first <= target && this->second >= target;
}


template <class T>
bool Range<T>::isDisjointFrom(const Range<T>& target) const {
    return this->second < target.first || this->first > target.second;
}


template <class T>
std::string Range<T>::toStr() const {
    std::stringstream ss;
    ss << this->first << "-" << this->second;
    return ss.str();
}


template <class T>
ustring Range<T>::toUstr() const {
    return ::utils::toUstr(this->toStr());
}


template <class T>
Range<T> Range<T>::fromStr(const std::string& str) {
    std::smatch matches;
    if (!std::regex_search(str, matches, Range<T>::REGEX) || matches.size() != 3) {
        std::cerr << "Error: Range::fromStr(): bad string \"" << str << "\" passed" << std::endl
                  << "Regex to match is \"" << Range<T>::REGEX_STR << "\"; matched groups are:"
                  << std::endl;
        for (auto match : matches) {
            std::cerr << match.str() << std::endl;
        }
        std::exit(EXIT_FAILURE);
    }

    Range<T> range;
    range.first = T(std::stoi(matches[1].str()));
    range.second = T(std::stoi(matches[2].str()));
    return range;
}


template <class T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toStr();
}


template class Range<Kw>;
//template class Range<IdAlias>;

template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
//template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);
