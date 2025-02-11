#pragma once

#include <map>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

static const int KEY_SIZE   = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE    = 128 / 8;

////////////////////////////////////////////////////////////////////////////////
// ustring
////////////////////////////////////////////////////////////////////////////////

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
using ustring = std::basic_string<unsigned char>;

ustring toUstr(int n);
ustring toUstr(std::string s);
ustring toUstr(unsigned char* p, int len);

std::ostream& operator << (std::ostream& os, const ustring str);

////////////////////////////////////////////////////////////////////////////////
// IEncryptable
////////////////////////////////////////////////////////////////////////////////

template <typename T>
class IEncryptable {
    protected:
        T val;

    public:
        IEncryptable(T val);
        T get();

        virtual ustring toUstr() = 0;
};

class Id : public IEncryptable<int> {
    public:
        Id(int val);

        ustring toUstr() override;

        static Id fromUstr(ustring ustr);
        friend void operator ++ (Id& id, int _); // `int _` required to mark as postfix
        friend bool operator + (const Id& id1, const Id& id2);
        friend bool operator - (const Id& id1, const Id& id2);
        friend bool operator == (const Id& id1, const Id& id2);
        friend bool operator < (const Id& id1, const Id& id2);
        friend bool operator > (const Id& id1, const Id& id2);
        friend bool operator <= (const Id& id1, const Id& id2);
        friend bool operator >= (const Id& id1, const Id& id2);
        friend std::ostream& operator << (std::ostream& os, const Id& id);
};

// todo move to srci files?
//class SrciAuxIndVal : public IEncryptable<std::pair<KwRange, IdRange>> {
//    public:
//        ustring toUstr();
//        static std::pair<KwRange, IdRange> fromUstr(ustring ustr);
//}

template <typename T>
ustring toUstr(IEncryptable<T>& iEncryptable);

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

// for generality, all keywords are ranges; single keywords are just size 1 ranges
template <typename T>
class Range : public std::pair<T, T> {
    public:
        Range();
        Range(const T& start, const T& end);

        T size();
        bool contains(Range<T> range);
        bool isDisjointWith(Range<T> range);

        template<typename T2>
        friend std::ostream& operator << (std::ostream& os, const Range<T2>& range);
};

using Kw      = int;
using IdRange = Range<Id>;
using KwRange = Range<Kw>;

template <typename T>
ustring toUstr(Range<T> range);

////////////////////////////////////////////////////////////////////////////////
// Other Custom Types
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (because screw you Log-SRC-i for making everything a nonillion times more complicated)
// no easy way to enforce (templated) base classes for clarity unfortunately, like Java generics `extends`
// so just make sure `DbDocType` subclasses `IEncryptable` and `DbKwType` subclasses `Range`
template <typename DbDocType, typename DbKwType>
using Db         = std::vector<std::tuple<DbDocType, DbKwType>>; // todo test if list is faster
// changing `Doc` will lead to a snowball of broken dreams...
using Doc        = std::tuple<Id, KwRange>; // todo needed?? can just do make_tuple??
//                `std::map<label, std::pair<data, iv>>`
using EncInd     = std::map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

// i know this should probably be in a separate file but i wanted to cut down on file count
// so it was clearer what does what

template <typename T>
class TdagNode {
    private:
        Range<T> range;
        TdagNode<T>* left;
        TdagNode<T>* right;
        TdagNode<T>* extraParent;

        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::list<TdagNode<T>*> traverse();
        std::list<TdagNode<T>*> traverse(std::unordered_set<TdagNode<T>*>& extraParents);

    public:
        /**
         * Construct a `TdagNode` with the given `Range`, leaving its children `nullptr`.
         */
        TdagNode(const Range<T>& range);

        /**
         * Construct a `TdagNode` with the given children, setting its own `range`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode<T>* left, TdagNode<T>* right);
        
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode<T>* findSrc(Range<T> targetRange);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::list<Range<T>> traverseLeaves();

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with `leafRange` within the tree `this`,
         * including the leaf itself.
         */
        std::list<TdagNode<T>*> getLeafAncestors(Range<T> leafRange);

        /**
         * Get `this->range`.
         */
        Range<T> getRange();

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given maximum leaf value,
         * with consecutive size 1 ranges as leaves.
         */
        static TdagNode<T>* buildTdag(T maxLeafVal);

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but contiguous `Range`s; they are sorted in
         * ascending order by `set` based on the definition of the `<` operator for `Range`.
         */
        static TdagNode<T>* buildTdag(std::set<Range<T>> leafVals);

        template <typename T2>
        friend std::ostream& operator << (std::ostream& os, TdagNode<T2> node);
};

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors();

ustring prf(ustring key, ustring input);

ustring genIv();
ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv = ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv = ustring());
