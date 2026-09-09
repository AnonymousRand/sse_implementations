#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <iostream>
#include <regex>
#include <string>

#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


/**
 * preconditions:
 *     - range end is greater than or equal to range start.
 */
template <std::integral T>
struct Range {
public:
    T start;
    T end;

    // we should be able to use aggregated initialization here

    static const Range DUMMY() {
        return Range {::DUMMY, ::DUMMY};
    }
    const bool isDummy() const {
        return *this == DUMMY();
    }

    //--------------------------------------------------------------------------

    T size() const;
    bool contains(const Range& target) const;
    bool contains(T target) const;
    bool isDisjointFrom(const Range& target) const;

    std::string toStr() const;
    ustring toUstr() const;
    static Range fromStr(const std::string& str);

    friend bool operator ==(const Range& range1, const Range& range2) = default;
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
