#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/ustring.h"


//==============================================================================
// types
//==============================================================================


using Kw      = int64_t;
using Id      = int64_t;
using IdAlias = int64_t; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)


/**
 * preconditions:
 *     - keywords and ids are both nonnegative integer values (storable by `int64_t`)
 *       (as `DUMMY` here is used for both).
 */
inline constexpr int64_t DUMMY = -1;


// forward declarations (not includes to avoid circular includes)
template <class T>
class Range;

template <class T, class DbKw>
class IDbDoc;

template <class DbKw = Kw>
class Doc;


// black magic to detect if `T` is derived from `IDbDoc` regardless of `IDbDoc`'s template param
// i.e. without needing to know what the template param `T2` of `IDbDoc` is, unlike `std::derived_from` for example
// (Java generics `extends`: look what they need to mimic a fraction of my power)
// (and this doesn't even enforce existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IsDbDoc = requires(T t) {
    []<class ... Args>(IDbDoc<Args ...>&){}(t);
};


// this enforces the above plus that `T` uses `DbKw` as its second template param, e.g.
// `IsDbDoc<IDbDoc<A, int64_t>, int64_t>` passes but not `IdDbDoc<IDbDoc<A, char>, int64_t>`
template <class T, class DbKw>
concept IsValidDbParams = requires(T t) {
    []<class T2>(IDbDoc<T2, DbKw>&){}(t);
};


// allow polymorphic types for DB (since Log-SRC-i exists)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
using DbEntry     = std::pair<DbDoc, Range<DbKw>>;
// technically dbs only need to contain the `DbDoc` part since `Doc` is the full (id,kw,op) tuple
// but we will also explicitly store keyword ranges (`DbKw`) for convenience in our implementation
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
using Db          = std::vector<DbEntry<DbDoc, DbKw>>;
template <class IndKey = Kw, class DbDoc = Doc<>>
using Ind         = std::unordered_map<Range<IndKey>, std::vector<DbDoc>>;


enum class Op : char {
    INS   = 'I',
    DEL   = 'D',
    DUMMY = '-'
};


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
std::ostream& operator <<(std::ostream& os, const DbEntry<DbDoc, DbKw>& dbEntry);


//==============================================================================
// util functions
//==============================================================================


namespace utils {


template <class IndKey, IsDbDoc DbDoc>
void shuffleInd(Ind<IndKey, DbDoc>& ind);


template <IsDbDoc DbDoc, class DbKw>
Range<DbKw> findDbKwBounds(const Db<DbDoc, DbKw>& db);


template <IsDbDoc DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db);


template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& docs);


uint64_t hashToPos(const ustring& hash);


} // namespace `utils`
