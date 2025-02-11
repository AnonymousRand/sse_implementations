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

ustring toUstr(std::string s);
ustring toUstr(unsigned char* p, int len);

std::ostream& operator << (std::ostream& os, const ustring str);

template <typename T>
class IEncryptable {
    protected:
        T val;

    public:
        using Type = T;

        virtual ustring toUstr() = 0;
        virtual IEncryptable<T> fromUstr(ustring ustr) = 0;
        T get();
};

class Id : public IEncryptable<int> {
    public:
        ustring toUstr();
        Id fromUstr(ustring ustr);
}

// todo move to srci files?
//class SrciAuxIndVal : public IEncryptable<std::pair<KwRange, IdRange>> {
//    public:
//        ustring toUstr();
//        std::pair<KwRange, IdRange> fromUstr(ustring ustr);
//}

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

// changing `Kw` will lead to a snowball of broken dreams...
using Kw = int;

// for generality, all keywords are ranges; single keywords are just size 1 ranges
template <typename RangeType>
class Range : public std::pair<RangeType, RangeType> {
    public:
        using Type = RangeType;

        RangeType size();
        bool contains(Range<RangeType> range);
        bool isDisjointWith(Range<RangeType> range);
        ustring toUstr();

        // todo this might need template by itself
        friend std::ostream& operator << (std::ostream& os, const Range<RangeType>& range);
};

using IdRange = Range<Id>;
using KwRange = Range<Kw>;

////////////////////////////////////////////////////////////////////////////////
// Other Custom Types
////////////////////////////////////////////////////////////////////////////////

// allow polymophic document types for db (because screw you Log-SRC-i for making everything a nonillion times more complicated)
template <typename DbDocType, DbKwType>
// TODO ok probably just give up and make separate cases for logsrci???
using Db         = std::vector<std::tuple<DbDocType, DbKwType>>; // todo test if list is faster
using Doc        = std::tuple<Id, KwRange>;
//                `std::map<label, std::pair<data, iv>>`
using EncInd     = std::map<ustring, std::pair<ustring, ustring>>;
using QueryToken = std::pair<ustring, ustring>;

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

// i know this should probably be in a separate file but i wanted to cut down on file count
// so it was clearer what does what

class TdagNode {
    private:
        Range range;
        TdagNode* left;
        TdagNode* right;
        TdagNode* extraParent;

        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::list<TdagNode*> traverse();
        std::list<TdagNode*> traverse(std::unordered_set<TdagNode*>& extraParents);

    public:
        /**
         * Construct a `TdagNode` with the given `Range`, leaving its children `nullptr`.
         */
        TdagNode(Range range);

        /**
         * Construct a `TdagNode` with the given children, setting its own `range`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode* left, TdagNode* right);
        
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode* findSrc(Range targetRange);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::list<Range> traverseLeaves();

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with `leafRange` within the tree `this`,
         * including the leaf itself.
         */
        std::list<TdagNode*> getLeafAncestors(Range leafRange);

        /**
         * Get `this->range`.
         */
        Range getRange();

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given maximum leaf value,
         * with consecutive size 1 ranges as leaves.
         */
        static TdagNode* buildTdag(int maxLeafVal);

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but contiguous `Range`s; they are sorted in
         * ascending order by `set` based on the definition of the `<` operator for `Range`.
         */
        static TdagNode* buildTdag(std::set<Range> leafVals);

        friend std::ostream& operator << (std::ostream& os, TdagNode* node);
};

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors();

ustring prf(ustring key, ustring input);

ustring genIv();
ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv = ustring());
ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv = ustring());
