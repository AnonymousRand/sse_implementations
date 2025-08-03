#pragma once


#include <concepts>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <openssl/evp.h>


/******************************************************************************/
/* Constants/Configs                                                          */
/******************************************************************************/


using uchar = unsigned char;
using ulong = unsigned long;

// lengths are in bytes
static constexpr int KEY_LEN         = 256 / 8;
static constexpr int IV_LEN          = 128 / 8;
static constexpr int BLOCK_SIZE      = 128 / 8;
static constexpr int HASH_OUTPUT_LEN = 512 / 8;
static const EVP_CIPHER* ENC_CIPHER  = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC       = EVP_sha512();

/**
 * Precondition:
 *     - Keywords and ids are both nonnegative integral values (storable by `long`)
 *       (as `DUMMY` is used for both).
 */
static constexpr long DUMMY = -1;

static std::random_device RAND_DEV;
static std::mt19937 RNG(RAND_DEV());


/******************************************************************************/
/* Basic Declarations                                                         */
/******************************************************************************/


using Kw      = long;
using Id      = long;
using IdAlias = long; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)

// forward declarations
template <class T>
class Range;
template <class T, class DbKw>
class IDbDoc;
template <class DbKw = Kw>
class Doc;
enum class Op : char {
    INS   = 'I',
    DEL   = 'D',
    DUMMY = '-'
};

// black magic to detect if `T` is derived from `IDbDoc` regardless of `IDbDoc`'s template param
// i.e. without needing to know what the template param `T2` of `IDbDoc` is, unlike `std::derived_from` for example
// (Java generics `extends`: look what they need to mimic a fraction of my power)
// (and this doesn't even enforce existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IsDbDoc = requires(T t) {
    []<class ... Args>(IDbDoc<Args ...>&){}(t);
};

// this enforces the above plus that `T` uses `DbKw` as its second template param, e.g.
// `IsDbDoc<IDbDoc<A, long>, long>` passes but not `IdDbDoc<IDbDoc<A, char>, long>`
template <class T, class DbKw>
concept IsValidDbParams = requires(T t) {
    []<class T2>(IDbDoc<T2, DbKw>&){}(t);
};

// allow polymorphic types for DB (since Log-SRC-i exists)
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
using DbEntry = std::pair<DbDoc, Range<DbKw>>;
// technically dbs only need to contain the `DbDoc` part since `Doc` is the full (id,kw,op) tuple
// but we will also explicitly store keyword ranges (`DbKw`) for convenience in our implementation
template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
using Db      = std::vector<DbEntry<DbDoc, DbKw>>;
template <class IndK = Kw, class DbDoc = Doc<>>
using Ind     = std::unordered_map<Range<IndK>, std::vector<DbDoc>>;


/******************************************************************************/
/* `ustring`                                                                  */
/******************************************************************************/


// use `ustring` instead of `uchar*` to avoid hell
using ustring = std::basic_string<uchar>;

ustring toUstr(long n);
ustring toUstr(const std::string& s);
ustring toUstr(uchar* ucstr, int len);
std::string toStr(const ustring& ustr);

std::ostream& operator <<(std::ostream& os, const ustring& ustr);

// provide hash function for `ustring`s to use faster hashmap-based structures, like `unordered_map` instead of `map`
template <>
struct std::hash<ustring> {
    inline std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(toStr(ustr));
    }
};


/******************************************************************************/
/* `Range`                                                                    */
/******************************************************************************/


/**
 * Precondition:
 *     - Range end is greater than or equal to range start.
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


template <>
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


/******************************************************************************/
/* `IDbDoc`                                                                   */
/******************************************************************************/


// interface for documents in dataset; also store the `DbKw` range (e.g. Log-SRC replications) they're stored with
// in their respective datasets, so that we can easily fetch them in plaintext for things like SDa
// (otherwise they might be only accessible via the encrypted "label" in the encrypted index, which can be a hash/PRF
// and hence not easily reversible, unlike `DbDoc`s which are just encrypted and can be easily decrypted)
template <class T, class DbKw>
class IDbDoc {
    public:
        IDbDoc() = default;
        IDbDoc(const T& val, const Range<DbKw>& dbKwRange);

        T get() const;
        Range<DbKw> getDbKwRange() const;
        virtual std::string toStr() const = 0;
        ustring toUstr() const;

        template <class T2, class DbKw2>
        friend std::ostream& operator <<(std::ostream& os, const IDbDoc<T2, DbKw2>& iDbDoc);

    protected:
        T val;
        Range<DbKw> dbKwRange;
};


/******************************************************************************/
/* `Doc`                                                                      */
/******************************************************************************/


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
template <class DbKw>
class Doc : public IDbDoc<std::tuple<Id, Kw, Op>, DbKw> {
    public:
        Doc() = default;
        Doc(const std::tuple<Id, Kw, Op>& val, const Range<DbKw>& dbKwRange);
        Doc(Id id, Kw kw, Op op, const Range<DbKw>& dbKwRange);

        std::string toStr() const override;
        static Doc<DbKw> fromUstr(const ustring& ustr);

        Id getId() const;
        Kw getKw() const;
        Op getOp() const;

    private:
        static const std::string REGEX_STR;
        static const std::regex REGEX;
};


template <>
struct std::hash<Doc<Kw>> {
    inline std::size_t operator ()(const Doc<Kw>& doc) const noexcept {
        return std::hash<std::string>{}(doc.toStr());
    }
};

// commented out since currently `Kw` and `IdAlias` are the same type (`long`)
//template <>
//struct std::hash<Doc<IdAlias>> {
//    inline std::size_t operator ()(const Doc<IdAlias>& doc) const noexcept {
//        return std::hash<std::string>{}(doc.toStr());
//    }
//};


/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/


class SrcIDb1Doc : public IDbDoc<std::pair<Kw, Range<IdAlias>>, Kw> {
    public:
        SrcIDb1Doc() = default;
        SrcIDb1Doc(const std::pair<Kw, Range<IdAlias>>& val, const Range<Kw>& kwRange);
        SrcIDb1Doc(Kw kw, const Range<IdAlias>& idAliasRange, const Range<Kw>& kwRange);

        std::string toStr() const override;
        static SrcIDb1Doc fromUstr(const ustring& ustr);

    private:
        static const std::string REGEX_STR;
        static const std::regex REGEX;
};


/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/


template <class IndK, IsDbDoc DbDoc>
void shuffleInd(Ind<IndK, DbDoc>& ind);

template <IsDbDoc DbDoc, class DbKw>
Range<DbKw> findDbKwBounds(const Db<DbDoc, DbKw>& db);

template <IsDbDoc DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db);

template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& docs);


/******************************************************************************/
/* Debugging                                                                  */
/******************************************************************************/


std::string strToHex(const uchar* str, int len);

std::string strToHex(const ustring& str);
