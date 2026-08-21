#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


/**
 * preconditions:
 *     - range end is greater than or equal to range start.
 */
template <std::integral T>
class Range : public std::pair<T, T> {
public:
    inline static const Range DUMMY() {
        return Range {::DUMMY, ::DUMMY};
    }

    static const bool isDummy(const Range& range) {
        return range == DUMMY();
    }

    //--------------------------------------------------------------------------

    Range() = default;
    Range(T start, T end);

    T size() const;
    bool contains(const Range& target) const;
    bool contains(T target) const;
    bool isDisjointFrom(const Range& target) const;

    std::string toStr() const;
    ustring toUstr() const;
    static Range fromStr(const std::string& str);

    template <std::integral T2>
    friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);

private:
    static const std::string REGEX_STR;
    static const std::regex REGEX;
};


// specialize `std::hash` for `Range` so that they can be used as keys for `std::unordered_*`
template <std::integral T>
struct std::hash<Range<T>> {
    inline std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<std::string>{}(range.toStr());
    }
};


// specialize `std::formatter` for `Range` so that they can be insert in `std::format()`
template <std::integral T>
struct std::formatter<Range<T>> : std::formatter<std::string> {
    // inherit `parse()` from std::string

    auto format(const Range<T>& range, std::format_context& ctx) const {
        return std::formatter<std::string>::format(range.toStr(), ctx);
    }
};
