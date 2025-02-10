#pragma once

#include <map>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

static const int KEY_SIZE = 256 / 8;
static const int BLOCK_SIZE = 128 / 8;
static const int IV_SIZE = 128 / 8;

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

// use `ustring` as much as possible instead of `unsigned char*` to avoid C-style hell
typedef std::basic_string<unsigned char> ustring;
typedef int                              Id;
typedef int                              Kw;
typedef std::pair<Id, Id>                IdRange;

// we generalize keywords to always be ranges; single keywords are just size 1 ranges
class KwRange {
    public:
        int start;
        int end;

        KwRange() = default;
        KwRange(int start, int end);
        int size();
        bool contains(KwRange kwRange);
        bool isDisjoint(KwRange kwRange);

        friend bool operator < (const KwRange& kwRange1, const KwRange& kwRange2);
        friend std::ostream& operator << (std::ostream& os, const KwRange& kwRange);
};

typedef std::tuple<Id, KwRange>                        Doc;
typedef std::vector<Doc>                               Db; // todo test if list is faster
typedef std::pair<ustring, ustring>                    QueryToken;
// `std::map<label, std::pair<data, iv>>`
typedef std::map<ustring, std::pair<ustring, ustring>> EncInd;

int ustrToInt(ustring n);
ustring toUstr(int n);
ustring toUstr(KwRange kwRange);
ustring toUstr(std::string s);
ustring toUstr(unsigned char* p, int len);

std::ostream& operator << (std::ostream& os, const ustring str);

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

// i know this should probably be in a separate file but i wanted to cut down on file count
// so it was clearer what does what
class TdagNode {
    private:
        KwRange kwRange;
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
         * Construct a `TdagNode` with the given `KwRange`, leaving its children `nullptr`.
         */
        TdagNode(KwRange kwRange);

        /**
         * Construct a `TdagNode` with the given children, setting its own `kwRange`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode* left, TdagNode* right);
        
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode* findSrc(KwRange targetKwRange);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::list<KwRange> traverseLeaves();

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with `leafKwRange` within the tree `this`,
         * including the leaf itself.
         */
        std::list<TdagNode*> getLeafAncestors(KwRange leafKwRange);

        /**
         * Get `this->kwRange`.
         */
        KwRange getKwRange();

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given maximum leaf value,
         * with consecutive size 1 ranges as leaves.
         */
        static TdagNode* buildTdag(Kw maxLeafVal);

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but contiguous `KwRange`s; they are sorted in
         * ascending order by `set` based on the definition of the `<` operator for `KwRange`.
         */
        static TdagNode* buildTdag(std::set<KwRange> leafVals);

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
