#include "utils/types/range.h"

#include <concepts>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "utils/str.h"
#include "utils/types/basic_types.h"
#include "utils/types/ustring.h"


template <std::integral T>
T Range<T>::size() const {
    // `+ 1` as both ends are inclusive
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
ustring Range<T>::encode() const {
    ustring ret = utils::str::encodeBigint(this->start);
    ret += utils::str::encodeBigint(this->end);
    return ret;
}


template <std::integral T>
int Range<T>::ENCODING_LEN = 2 * config::INT_MAX_BYTES;


template <std::integral T>
Range<T> Range<T>::decode(const ustring& encoding, int startIndex) {
    T start = utils::str::decodeBigint(encoding, startIndex);
    T end = utils::str::decodeBigint(encoding, startIndex + config::INT_MAX_BYTES);
    return Range<T> {start, end};
}


template <std::integral T>
std::string Range<T>::toPrettyStr() const {
    return std::format("{}-{}", this->start, this->end);
}


template <std::integral T>
bool operator <(const Range<T>& range1, const Range<T>& range2) {
    if (range1.start < range2.start) {
        return true;
    } else if (range1.start > range2.start) {
        return false;
    } else {
        return range1.end < range2.end;
    }
}


template <std::integral T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.toPrettyStr();
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class Range<Kw>;
//template class Range<IdAlias>;


template bool operator <(const Range<Kw>& range1, const Range<Kw>& range2);
//template bool operator <(const Range<IdAlias>& range1, const Range<IdAlias>& range2);


template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);
//template std::ostream& operator <<(std::ostream& os, const Range<IdAlias>& range);
