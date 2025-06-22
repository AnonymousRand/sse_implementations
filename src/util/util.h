#pragma once

#include <concepts>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <openssl/evp.h>

/******************************************************************************/
/* Constants/Configs                                                          */
/******************************************************************************/

// lengths are in bytes
static const int KEY_LEN            = 256 / 8;
static const int IV_LEN             = 128 / 8;
static const int BLOCK_SIZE         = 128 / 8;
static const EVP_CIPHER* ENC_CIPHER = EVP_aes_256_cbc();
static const EVP_MD* HASH_FUNC      = EVP_sha512();
static const int HASH_OUTPUT_LEN    = 512 / 8;
static const int ENC_IND_KEY_LEN    = 512 / 8;        // both PRF (default) and hash (res-hiding) have 512 bit output
static const int ENC_IND_DOC_LEN    = 3 * BLOCK_SIZE; // so max keyword/id size ~10^23 for encoding to fit
static const int ENC_IND_VAL_LEN    = ENC_IND_DOC_LEN + IV_LEN;
static const int ENC_IND_KV_LEN     = ENC_IND_KEY_LEN + ENC_IND_VAL_LEN;

/**
 * PRECONDITION: keywords are always positive.
 */
static const int DB_KW_MIN = 0;
static const int DUMMY     = -1;

/******************************************************************************/
/* Basic Declarations                                                         */
/******************************************************************************/

static std::random_device RAND_DEV;
static std::mt19937 RNG(RAND_DEV());

// use `ustring` as much as possible instead of `unsigned char*` to avoid hell
using ustring = std::basic_string<unsigned char>;
using Kw      = int;

template <class T> class Range;
template <class T> class IDbDoc;
template <class T> class IMainDbDoc;
class Id;
class Op;
class IdOp;
template <class DbKw = Kw> class SrcIDb1Doc;

// black magic to detect if template param `T` is a specialization of `IDbDoc`/is derived from one
// (https://stackoverflow.com/a/71921982)
template <class T> concept IDbDoc_ = requires(T t) {
    []<class X>(IDbDoc<X>&){}(t);
};
template <class T> concept IMainDbDoc_ = requires(T t) {
    []<class X>(IMainDbDoc<X>&){}(t);
};

// allow polymorphic types for DB (id vs. (id, op) documents, Log-SRC-i etc.)
template <IDbDoc_ DbDoc = IdOp, class DbKw = Kw> 
using DbEntry = std::pair<DbDoc, Range<DbKw>>;
template <IDbDoc_ DbDoc = IdOp, class DbKw = Kw>
using Db      = std::vector<DbEntry<DbDoc, DbKw>>;
template <class DbKw = Kw, class DbDoc = IdOp>
using Ind     = std::unordered_map<Range<DbKw>, std::vector<DbDoc>>;

/******************************************************************************/
/* `ustring`                                                                  */
/******************************************************************************/

ustring toUstr(int n);
ustring toUstr(const std::string& s);
ustring toUstr(unsigned char* p, int len);
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
 * PRECONDITION: range end is greater than or equal to range start.
 */
template <class T>
class Range : public std::pair<T, T> {
    public:
        Range();
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
        virtual ustring encode() const;
        virtual std::string toStr() const = 0;

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, const IDbDoc<T2>& iEncryptable);
};

template <class T>
class IMainDbDoc : public IDbDoc<T> {
    public:
        using IDbDoc<T>::IDbDoc;

        virtual Id getId() const = 0;
};

/******************************************************************************/
/* `Id`                                                                       */
/******************************************************************************/

class Id : public IMainDbDoc<int> {
    public:
        Id() = default;
        Id(int val);

        static Id decode(const ustring& ustr);
        std::string toStr() const override;

        Id getId() const override;

        static Id fromStr(const std::string& str);
        Id& operator ++();
        Id operator ++(int); // unused `int` param marks `++` as postfix
        Id& operator +=(const Id& id);
        Id& operator -=(const Id& id);
        Id& operator +=(int n);
        Id& operator -=(int n);
        friend Id operator +(Id id1, const Id& id2);
        friend Id operator +(Id id, int n);
        friend Id operator -(Id id1, const Id& id2);
        friend Id operator -(Id id, int n);
        friend bool operator ==(const Id& id1, const Id& id2);
        friend bool operator ==(const Id& id1, int n);
        friend bool operator <(const Id& id1, const Id& id2);
        friend bool operator >(const Id& id1, const Id& id2);
        friend bool operator <=(const Id& id1, const Id& id2);
        friend bool operator >=(const Id& id1, const Id& id2);
};

template <>
struct std::hash<Id> {
    std::size_t operator ()(const Id& id) const noexcept {
        return std::hash<std::string>{}(id.toStr());
    }
};

// id aliases are functionally identical to ids, but it's still nice to have this layer of abstraction for clarity
using IdAlias = Id;

/******************************************************************************/
/* `Op`                                                                       */
/******************************************************************************/

class Op {
    private:
        std::string val;

    public:
        Op() = default;
        Op(const std::string& val);

        std::string toStr() const;

        static Op fromStr(const std::string& val);
        friend bool operator ==(const Op& op1, const Op& op2);
        friend std::ostream& operator <<(std::ostream& os, const Op& Op);
};

const Op INSERT("INSERT");
const Op DELETE("DELETE");

/******************************************************************************/
/* `IdOp`                                                                     */
/******************************************************************************/

class IdOp : public IMainDbDoc<std::pair<Id, Op>> {
    public:
        IdOp() = default;
        IdOp(const Id& id);
        IdOp(const Id& id, const Op& op);

        static IdOp decode(const ustring& ustr);
        std::string toStr() const override;

        Id getId() const override;

        friend bool operator <(const IdOp& doc1, const IdOp& doc2);
        friend bool operator ==(const IdOp& doc1, const IdOp& doc2);
};

std::vector<IdOp> removeDeletedIdOps(const std::vector<IdOp>& idOps);

/******************************************************************************/
/* `SrcIDb1Doc`                                                               */
/******************************************************************************/

template <class DbKw>
class SrcIDb1Doc : public IDbDoc<std::pair<Range<DbKw>, Range<IdAlias>>> {
    public:
        SrcIDb1Doc() = default;
        SrcIDb1Doc(const Range<DbKw>& dbKwRange, const Range<IdAlias>& idAliasRange);

        static SrcIDb1Doc<DbKw> decode(const ustring& ustr);
        std::string toStr() const override;
};

/******************************************************************************/
/* SSE Utils                                                                  */
/******************************************************************************/

template <class DbKw, class DbDoc>
void shuffleInd(Ind<DbKw, DbDoc>& ind);

template <class DbDoc, class DbKw>
DbKw findMaxDbKw(const Db<DbDoc, DbKw>& db);

template <class DbDoc, class DbKw>
std::unordered_set<Range<DbKw>> getUniqDbKwRanges(const Db<DbDoc, DbKw>& db);
