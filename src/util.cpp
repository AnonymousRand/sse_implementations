#include <cmath>
#include <memory>
#include <iostream>
#include <sstream>
#include <string.h>

#include "util.h"

/* TODO
 * >finish change of as much as possible kwrange -> kw
        >>make Db templated
        also means kwrange cant be a class anymore! need to be a pair. in fact enforce dbtypekw has base class pair
        also need to make tdags templated!!! to store dbtypekw
    merge setup and buildindex again? since thats what dynamic paper does
    const as much in tdag/util as possible?
    review class slides for srci as well
*/

////////////////////////////////////////////////////////////////////////////////
// Custom Types
////////////////////////////////////////////////////////////////////////////////

int ustrToInt(ustring s) {
    std::string str = std::string(s.begin(), s.end());
    return std::stoi(str);
}

ustring toUstr(int n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}

ustring toUstr(IdRange idRange) {
    return toUstr(idRange.first) + toUstr("-") + toUstr(idRange.second);
}

ustring toUstr(KwRange kwRange) {
    return toUstr(kwRange.start) + toUstr("-") + toUstr(kwRange.end);
}

ustring toUstr(std::string s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring toUstr(unsigned char* p, int len) {
    return ustring(p, len);
}

KwRange::KwRange(Kw start, Kw end) {
    this->start = start;
    this->end = end;
}

Kw KwRange::size() {
    return (Kw)abs(this->end - this->start);
}

bool KwRange::contains(KwRange kwRange) {
    return this->start <= kwRange.start && this->end >= kwRange.end;
}

bool KwRange::isDisjoint(KwRange kwRange) {
    return this->end < kwRange.start || this->start > kwRange.end;
}

// implemented the same way as `std::pair` does to ensure that this can reflexively determine equivalence
bool operator < (const KwRange& kwRange1, const KwRange& kwRange2) {
    if (kwRange1.start == kwRange2.start) {
        return kwRange1.end < kwRange2.end;
    }
    return kwRange1.start < kwRange2.start;
}

std::ostream& operator << (std::ostream& os, const KwRange& kwRange) {
    os << kwRange.start << "-" << kwRange.end;
    return os;
}

std::ostream& operator << (std::ostream& os, const ustring str) {
    for (auto c : str) {
        os << static_cast<char>(c);
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

TdagNode::TdagNode(KwRange kwRange) {
    this->kwRange = kwRange;
    this->left = nullptr;
    this->right = nullptr;
    this->extraParent = nullptr;
}

TdagNode::TdagNode(TdagNode* left, TdagNode* right) {
    this->kwRange = KwRange(left->kwRange.start, right->kwRange.end);
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
// track `extraParent` nodes to prevent duplicates; use `unordered_set` as it's probably the fastest way to do this
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
TdagNode* TdagNode::findSrc(KwRange targetKwRange) {
    std::map<Kw, TdagNode*> srcCandidates;
    auto addCandidate = [&](TdagNode* node) {
        if (node == nullptr || !node->kwRange.contains(targetKwRange)) {
            return -1;
        }

        Kw diff = (targetKwRange.start - node->kwRange.start) + (node->kwRange.end - targetKwRange.end);
        srcCandidates[diff] = node;
        return diff;
    };

    // if the current node is disjoint with the target range, it is impossible for
    // its children or extra TDAG parent to be the SRC, so we can early exit
    if (this->kwRange.isDisjoint(targetKwRange)) {
        return nullptr;
    }

    // else find best SRC between current node, best SRC in left subtree, best SRC in right subtree,
    // and extra TDAG parent 
    Kw diff = -1;
    if (this->extraParent != nullptr) {
        diff = addCandidate(this->extraParent);
        if (diff == 0) {
            return this->extraParent;
        }
    }
    // if the current node's range is more than one narrower than the target range, it is impossible for
    // its children to be the SRC, so we can early exit if we also know its extra TDAG parent cannot be an SRC
    if (diff == -1 && this->kwRange.size() < targetKwRange.size() - 1) {
        return nullptr;
    }

    diff = addCandidate(this);
    if (diff == 0) {
        return this;
    }
    if (this->left != nullptr) {
        TdagNode* leftSrc = this->left->findSrc(targetKwRange);
        diff = addCandidate(leftSrc);
        if (diff == 0) {
            return leftSrc;
        }
    }
    if (this->right != nullptr) {
        TdagNode* rightSrc = this->right->findSrc(targetKwRange);
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

std::list<KwRange> TdagNode::traverseLeaves() {
    std::list<KwRange> leafVals;
    std::list<TdagNode*> nodes = this->traverse();
    for (TdagNode* node : nodes) {
        if (node->left == nullptr && node->right == nullptr) {
            leafVals.push_back(node->kwRange);
        }
    }
    return leafVals;
}

std::list<TdagNode*> TdagNode::getLeafAncestors(KwRange leafKwRange) {
    std::list<TdagNode*> ancestors = {this};

    if (this->left != nullptr && this->left->kwRange.contains(leafKwRange)) {
        ancestors.splice(ancestors.end(), this->left->getLeafAncestors(leafKwRange));
    }
    if (this->right != nullptr && this->right->kwRange.contains(leafKwRange)) {
        ancestors.splice(ancestors.end(), this->right->getLeafAncestors(leafKwRange));
    }
    if (this->extraParent != nullptr && this->extraParent->kwRange.contains(leafKwRange)) {
        ancestors.push_back(this->extraParent);
    }

    return ancestors;
}

KwRange TdagNode::getKwRange() {
    return this->kwRange;
}

TdagNode* TdagNode::buildTdag(Kw maxLeafVal) {
    std::set<KwRange> leafVals;
    for (Kw i = 0; i <= maxLeafVal; i++) {
        leafVals.insert(KwRange(i, i));
    }
    return TdagNode::buildTdag(leafVals);
}

TdagNode* TdagNode::buildTdag(std::set<KwRange> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `TdagNode.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // list to hold nodes while building; initialize with leaves
    std::list<TdagNode*> l;
    for (KwRange leafVal : leafVals) {
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
            if (node2->kwRange.start - 1 == node1->kwRange.end) {
                // if `node1` is the left child of new parent node
                TdagNode* parent = new TdagNode(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->kwRange.end + 1 == node1->kwRange.start) {
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
        os << node->getKwRange() << std::endl;
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
