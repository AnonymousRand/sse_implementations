#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include "utils/constants.h"
#include "utils/sse_utils.h"
#include "utils/ustring.h"


/**
 * preconditions:
 *     - range end is greater than or equal to range start.
 */
template <class T>
class Range : public std::pair<T, T> {
    public:
        static const std::string REGEX_STR;

        Range() = default;
        Range(const T& start, const T& end);

        T size() const;
        bool contains(const Range<T>& target) const;
        bool contains(T target) const;
        bool isDisjointFrom(const Range<T>& target) const;

        std::string toStr() const;
        ustring toUstr() const;
        static Range<T> fromStr(const std::string& str);

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);

    private:
        static const std::regex REGEX;
};


template <class T>
struct std::hash<Range<T>> {
    inline std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<std::string>{}(range.toStr());
    }
};


template <class T>
static Range<T> DUMMY_RANGE() {
    return Range<T> {DUMMY, DUMMY};
}
