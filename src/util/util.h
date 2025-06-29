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
static constexpr int ENC_IND_KEY_LEN = 512 / 8;        // both PRF (default) and hash (res-hiding) have 512 bit output
static constexpr int ENC_IND_DOC_LEN = 3 * BLOCK_SIZE; // so max keyword/id size ~10^19 for encoding to fit (starting 0)
static constexpr int ENC_IND_VAL_LEN = ENC_IND_DOC_LEN + IV_LEN;
static constexpr int ENC_IND_KV_LEN  = ENC_IND_KEY_LEN + ENC_IND_VAL_LEN;
static const EVP_CIPHER* ENC_CIPHER  = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC       = EVP_sha512();

/**
 * Precondition: keywords and ids are both nonnegative integral values (storable by `ulong`).
 */
static constexpr ulong MIN_DB_KW = 0;
static constexpr ulong DUMMY     = -1;

static std::random_device RAND_DEV;
static std::mt19937 RNG(RAND_DEV());


/******************************************************************************/
/* Basic Declarations                                                         */
/******************************************************************************/


// forward declarations
template <class T>
class Range;
template <class T>
class IDbDoc;
class Doc;
enum class Op : char {
    INS = 'I',
    DEL = 'D'
};

using Kw      = ulong;
using Id      = ulong;
using IdAlias = ulong; // Log-SRC-i "id aliases" (i.e. index 2 nodes/keywords)

// black magic to detect if `T` is derived from `IDbDoc` regardless of `IDbDoc`'s template param
// i.e. without needing to know what the template param `X` of `IDbDoc` is, unlike `std::derived_from` for example
// from https://stackoverflow.com/a/71921982
// (Java generics `extends`: look what they need to mimic a fraction of my power)
// (and this doesn't even enforce existence of instance methods as clearly as Java, so just pretend that it does)
template <class T>
concept IDbDoc_ = requires(T t) {
    []<class X>(IDbDoc<X>&){}(t);
};

// allow polymorphic types for DB (since Log-SRC-i exists)
template <IDbDoc_ DbDoc = Doc, class DbKw = Kw> 
using DbEntry = std::pair<DbDoc, Range<DbKw>>;
// technically dbs only need to contain the `DbDoc` part since `Doc` is the full (id,kw,op) tuple
// but we will also explicitly store keyword ranges (`DbKw`) for convenience in our implementation
template <IDbDoc_ DbDoc = Doc, class DbKw = Kw>
using Db      = std::vector<DbEntry<DbDoc, DbKw>>;
template <class IndK, class DbDoc>
using Ind     = std::unordered_map<Range<IndK>, std::vector<DbDoc>>;


/******************************************************************************/
/* `ustring`                                                                  */
/******************************************************************************/


// use `ustring` instead of `uchar*` to avoid hell
using ustring = std::basic_string<uchar>;

ustring toUstr(long n);
ustring toUstr(const std::string& s);
ustring toUstr(uchar* p, int len);
std::string fromUstr(const ustring& ustr);

// provide hash function for `ustring`s to use faster hashmap-based structures, like `unordered_map` instead of `map`
template <>
struct std::hash<ustring> {
    std::size_t operator ()(const ustring& ustr) const noexcept {
        return std::hash<std::string>{}(fromUstr(ustr));
    }
};

std::ostream& operator <<(std::ostream& os, const ustring& ustr);


/******************************************************************************/
/* `Range`                                                                    */
/******************************************************************************/


/**
 * Precondition: range end is greater than or equal to range start.
 */
template <class T>
class Range : public std::pair<T, T> {
    public:
        Range() = default;
        Range(const T& start, const T& end);

        T size() const;
        bool contains(const Range<T>& target) const;
        bool isDisjointFrom(const Range<T>& target) const;

        std::string toStr() const;
        static Range<T> fromStr(const std::string& str);
        ustring toUstr() const;
        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, const Range<T2>& range);
};


template <>
template <class T>
struct std::hash<Range<T>> {
    std::size_t operator ()(const Range<T>& range) const noexcept {
        return std::hash<std::string>{}(range.toStr());
    }
};

template <class T>
static Range<T> DUMMY_RANGE() {
    return Range<T>(T(DUMMY), T(DUMMY));
}


/******************************************************************************/
/* `IDbDoc`                                                                   */
/******************************************************************************/


// interface for documents in dataset
template <class T>
class IDbDoc {
    protected:
        T val;

    public:
        IDbDoc() = default;
        IDbDoc(const T& val);

        T get() const;
        virtual ustring toUstr() const;
        virtual std::string toStr() const = 0;

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, const IDbDoc<T2>& iDbDoc);
};


/******************************************************************************/
/* `Doc`                                                                      */
/******************************************************************************/


// these are the "database tuples"; accommodate dynamic SSE by also storing the operation
class Doc : public IDbDoc<std::tuple<Id, Kw, Op>> {
    public:
        static const std::regex fromStrRegex;

        Doc() = default;
        Doc(Id id, Kw kw, Op op);

        std::string toStr() const override;
        static Doc fromUstr(const ustring& ustr);
        static Doc fromStr(const std::string& str);

        Id getId() const;
        Kw getKw() const;
        Op getOp() const;

        friend bool operator ==(const Doc& doc1, const Doc& doc2);
};


/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/


class SrcIDb1Doc : public IDbDoc<std::pair<Range<Kw>, Range<IdAlias>>> {
    public:
        static const std::regex fromStrRegex;

        SrcIDb1Doc() = default;
        SrcIDb1Doc(const Range<Kw>& kwRange, const Range<IdAlias>& idAliasRange);

        std::string toStr() const override;
        static SrcIDb1Doc fromUstr(const ustring& ustr);
};


/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/


template <class IndK, IDbDoc_ DbDoc>
void shuffleInd(Ind<IndK, DbDoc>& ind);

template <IDbDoc_ DbDoc, class DbKw>
DbKw findMaxDbKw(const Db<DbDoc, DbKw>& db);

template <IDbDoc_ DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db);

template <IDbDoc_ DbDoc>
void processResults(std::vector<DbDoc>& docs);
