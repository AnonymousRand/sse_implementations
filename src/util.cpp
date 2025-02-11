#include <cmath>
#include <memory>
#include <iostream>
#include <sstream>
#include <string.h>

#include "util.h"

/* TODO
    merge setup and buildindex again? since thats what dynamic paper does. check if wikipedia/2024 paper still do that, since it does make the code harder (have to return a pair)
    const as much in tdag/util as possible?
    review class slides for srci as well
*/

////////////////////////////////////////////////////////////////////////////////
// ustring
////////////////////////////////////////////////////////////////////////////////

ustring toUstr(std::string s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring toUstr(unsigned char* p, int len) {
    return ustring(p, len);
}

std::ostream& operator << (std::ostream& os, const ustring str) {
    for (auto c : str) {
        os << static_cast<char>(c);
    }
    return os;
}

template class IEncryptable<int>;
template class IEncryptable<std::pair<KwRange, IdRange>>;

template <typename T>
T IEncryptable<T>::get() {
    return this->val;
}

ustring Id::toUstr() {
    std::string str = std::to_string(this->val);
    return ustring(str.begin(), str.end());
}

Id Id::fromUstr(ustring ustr) {
    std::string str = std::string(s.begin(), s.end());
    this->val = std::stoi(str);
    return this;
}

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

template class Range<Id>;
template class Range<Kw>;

template <typename RangeType>
RangeType Range<RangeType>::size() {
    return (RangeType)abs(this->second - this->first);
}

template <typename RangeType>
bool Range<RangeType>::contains(Range<RangeType> range) {
    return this->first <= range.first && this->second >= range.second;
}

template <typename RangeType>
bool Range<RangeType>::isDisjointWith(Range<RangeType> range) {
    return this->second < range.first || this->first > range.second;
}

template <typename RangeType>
ustring Range<RangeType>::toUstr() {
    return toUstr(this->first) + toUstr("-") + toUstr(this->second);
}

template <typename RangeType>
std::ostream& operator << (std::ostream& os, const Range<RangeType>& range) {
    os << range.start << "-" << range.end;
    return os;
}

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

TdagNode::TdagNode(Range range) {
    this->range = range;
    this->left = nullptr;
    this->right = nullptr;
    this->extraParent = nullptr;
}

TdagNode::TdagNode(TdagNode* left, TdagNode* right) {
    this->range = Range(left->range.start, right->range.end);
    this->left = left;
    this->right = right;
    this->extraParent = nullptr;
}

TdagNode::~TdagNode() {
    if (this->left != nullptr) {
        delete this->left;
    }
    if (this->right != nullptr) {
        delete this->right;
    }
    if (this->extraParent != nullptr) {
        delete this->extraParent;
    }
}

// DFS preorder but with additional traversal of TDAG's extra parent nodes
// track `extraParent` nodes in an `unordered_set` to prevent duplicates
std::list<TdagNode*> TdagNode::traverse() {
    std::unordered_set<TdagNode*> nodes;
    return this->traverse(nodes);
}

std::list<TdagNode*> TdagNode::traverse(std::unordered_set<TdagNode*>& extraParents) {
    std::list<TdagNode*> nodes;
    nodes.push_front(this);

    if (this->left != nullptr) {
        nodes.splice(nodes.end(), this->left->traverse(extraParents));
    }
    if (this->right != nullptr) {
        nodes.splice(nodes.end(), this->right->traverse(extraParents));
    }
    if (this->extraParent != nullptr) {
        auto res = extraParents.insert(this->extraParent);
        // if insertion succeeded; i.e., the node is not already in the `unordered_set` (this prevents duplicates)
        if (res.second) {
            nodes.push_back(this->extraParent);
        }
    }

    return nodes;
}

// current algo uses divide-and-conquer with early exits to find best SRC
// TODO time complexity
// TODO note assumptions for optimizations: ranges strictly increasing, balanced tree
// TODO have to verify/prove early exits?
TdagNode* TdagNode::findSrc(Range targetRange) {
    std::map<int, TdagNode*> srcCandidates;
    auto addCandidate = [&](TdagNode* node) {
        if (node == nullptr || !node->range.contains(targetRange)) {
            return -1;
        }

        int diff = (targetRange.start - node->range.start) + (node->range.end - targetRange.end);
        srcCandidates[diff] = node;
        return diff;
    };

    // if the current node is disjoint with the target range, it is impossible for
    // its children or extra TDAG parent to be the SRC, so we can early exit
    if (this->range.isDisjointWith(targetRange)) {
        return nullptr;
    }

    // else find best SRC between current node, best SRC in left subtree, best SRC in right subtree,
    // and extra TDAG parent 
    int diff = -1;
    if (this->extraParent != nullptr) {
        diff = addCandidate(this->extraParent);
        if (diff == 0) {
            return this->extraParent;
        }
    }
    // if the current node's range is more than one narrower than the target range, it is impossible for
    // its children to be the SRC, so we can early exit if we also know its extra TDAG parent cannot be an SRC
    if (diff == -1 && this->range.size() < targetRange.size() - 1) {
        return nullptr;
    }

    diff = addCandidate(this);
    if (diff == 0) {
        return this;
    }
    if (this->left != nullptr) {
        TdagNode* leftSrc = this->left->findSrc(targetRange);
        diff = addCandidate(leftSrc);
        if (diff == 0) {
            return leftSrc;
        }
    }
    if (this->right != nullptr) {
        TdagNode* rightSrc = this->right->findSrc(targetRange);
        diff = addCandidate(rightSrc);
        if (diff == 0) {
            return rightSrc;
        }
    }

    if (srcCandidates.empty()) {
        return nullptr;
    } else {
        return srcCandidates.begin()->second; // take advantage of `std::map`s being sorted by key
    }
}

std::list<Range> TdagNode::traverseLeaves() {
    std::list<Range> leafVals;
    std::list<TdagNode*> nodes = this->traverse();
    for (TdagNode* node : nodes) {
        if (node->left == nullptr && node->right == nullptr) {
            leafVals.push_back(node->range);
        }
    }
    return leafVals;
}

std::list<TdagNode*> TdagNode::getLeafAncestors(Range leafRange) {
    std::list<TdagNode*> ancestors = {this};

    if (this->left != nullptr && this->left->range.contains(leafRange)) {
        ancestors.splice(ancestors.end(), this->left->getLeafAncestors(leafRange));
    }
    if (this->right != nullptr && this->right->range.contains(leafRange)) {
        ancestors.splice(ancestors.end(), this->right->getLeafAncestors(leafRange));
    }
    if (this->extraParent != nullptr && this->extraParent->range.contains(leafRange)) {
        ancestors.push_back(this->extraParent);
    }

    return ancestors;
}

Range TdagNode::getRange() {
    return this->range;
}

TdagNode* TdagNode::buildTdag(int maxLeafVal) {
    std::set<Range> leafVals;
    for (int i = 0; i <= maxLeafVal; i++) {
        leafVals.insert(Range(i, i));
    }
    return TdagNode::buildTdag(leafVals);
}

TdagNode* TdagNode::buildTdag(std::set<Range> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `TdagNode.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // list to hold nodes while building; initialize with leaves
    std::list<TdagNode*> l;
    for (Range leafVal : leafVals) {
        l.push_back(new TdagNode(leafVal));
    }

    // build full binary tree from leaves (this is my own algorithm i have no idea how good it is)
    // trees seem balanced though which is nice
    while (l.size() > 1) {
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        TdagNode* node1 = l.front();
        l.pop_front();
        // find a contiguous node
        for (auto it = l.begin(); it != l.end(); it++) {
            TdagNode* node2 = *it;
            if (node2->range.start - 1 == node1->range.end) {
                // if `node1` is the left child of new parent node
                TdagNode* parent = new TdagNode(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->range.end + 1 == node1->range.start) {
                // if `node2` is the left child of new parent node
                TdagNode* parent = new TdagNode(node2, node1);
                l.push_back(parent);
                l.erase(it);
                break;
            }
        }
    }

    // add extra TDAG nodes
    TdagNode* tdag = l.front();
    std::list<TdagNode*> nodes = tdag->traverse();
    while (!nodes.empty()) {
        TdagNode* node = nodes.front();
        nodes.pop_front();
        if (node->left == nullptr
                || node->right == nullptr
                || node->left->right == nullptr
                || node->right->left == nullptr) {
            continue;
        }

        TdagNode* extraParent = new TdagNode(node->left->right, node->right->left);
        node->left->right->extraParent = extraParent;
        node->right->left->extraParent = extraParent;
        // using my method of finding places to add extra nodes, extra nodes themselves must also be checked
        nodes.push_back(extraParent);
    }

    return tdag;
}

std::ostream& operator << (std::ostream& os, TdagNode* node) {
    std::list<TdagNode*> nodes = node->traverse();
    for (TdagNode* node : nodes) {
        os << node->getRange() << std::endl;
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////
// OpenSSL
////////////////////////////////////////////////////////////////////////////////

void handleOpenSslErrors() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

// PRF implemented with HMAC-SHA512, as done in Private Practical Range Search Revisited
ustring prf(ustring key, ustring input) {
    unsigned int outputLen;
    unsigned char* output = HMAC(EVP_sha512(), &key[0], key.length(), &input[0], input.length(), nullptr, &outputLen);
    return toUstr(output, outputLen);
}

ustring genIv() {
    unsigned char* iv = new unsigned char[IV_SIZE];
    int res = RAND_bytes(iv, IV_SIZE);
    if (res != 1) {
        handleOpenSslErrors();
    }
    ustring ustrIv = toUstr(iv, IV_SIZE);
    delete[] iv;
    return ustrIv;
}

ustring aesEncrypt(const EVP_CIPHER* cipher, ustring key, ustring ptext, ustring iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize encryption
    unsigned char* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleOpenSslErrors();
    }

    // perform encryption
    int ctextLen1, ctextLen2;
    ustring ctext;
    ctext.resize(ptext.length() + BLOCK_SIZE);
    if (EVP_EncryptUpdate(ctx, &ctext[0], &ctextLen1, &ptext[0], ptext.length()) != 1) {
        handleOpenSslErrors();
    }

    // finalize encryption (deal with last partial block)
    if (EVP_EncryptFinal_ex(ctx, &ctext[0] + ctextLen1, &ctextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ctext.resize(ctextLen1 + ctextLen2);
    return ctext;
}

ustring aesDecrypt(const EVP_CIPHER* cipher, ustring key, ustring ctext, ustring iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        handleOpenSslErrors();
    }

    // initialize decryption
    unsigned char* ucharIv;
    if (iv == ustring()) {
        ucharIv = nullptr;
    } else {
        ucharIv = &iv[0];
    }
    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, &key[0], ucharIv) != 1) {
        handleOpenSslErrors();
    }

    // perform decryption
    int ptextLen1, ptextLen2;
    ustring ptext;
    ptext.resize(ctext.length());
    if (EVP_DecryptUpdate(ctx, &ptext[0], &ptextLen1, &ctext[0], ctext.length()) != 1) {
        handleOpenSslErrors();
    }

    // finalize decryption (deal with last partial block)
    if (EVP_DecryptFinal_ex(ctx, &ptext[0] + ptextLen1, &ptextLen2) != 1) {
        handleOpenSslErrors();
    }

    EVP_CIPHER_CTX_free(ctx);
    ptext.resize(ptextLen1 + ptextLen2);
    return ptext;
}
