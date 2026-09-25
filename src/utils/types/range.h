#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <iostream>
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

    static Range DUMMY() { return Range {::DUMMY, ::DUMMY}; }
    bool isDummy() const { return *this == DUMMY(); }

    // default constructor needed for `IDbTuple` children's default constructors
    Range() = default;
    Range(T start, T end) : start(start), end(end) {}

    T size() const;
    bool contains(const Range& target) const;
    bool contains(T target) const;
    bool isDisjointFrom(const Range& target) const;

    ustring encode() const;
    static Range decode(const ustring& encoding, int startIndex = 0);
    std::string toPrettyStr() const;
    static int ENCOD_LEN; // can't make this `const` without initializing it in constructors

    friend bool operator ==(const Range& range1, const Range& range2) = default;
    template <std::integral T2>
    friend bool operator <(const Range<T2>& range1, const Range<T2>& range2);
    template <std::integral T2>
    friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
};


// specialize `std::hash` for `Range` so that they can be used as keys for `std::unordered_*`
template <std::integral T>
struct std::hash<Range<T>> {
    inline std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<ustring>{}(range.encode());
    }
};


// specialize `std::formatter` for `Range` so that it can be used in `std::format()`
template <std::integral T>
struct std::formatter<Range<T>> : std::formatter<std::string> {
    // inherit `parse()` from `std::string`

    auto format(const Range<T>& range, std::format_context& ctx) const {
        return std::formatter<std::string>::format(range.toPrettyStr(), ctx);
    }
};
