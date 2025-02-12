#include <cmath>
#include <memory>
#include <iostream>
#include <sstream>
#include <string.h>

#include "util.h"

/* TODO
    merge setup and buildindex again? since thats what dynamic paper does. check if wikipedia/2024 paper still do that, since it does make the code harder (have to return a pair)
    const as much in tdag/util as possible? like range constructor for example (copied from std::pair)
    review class slides for srci as well
*/

////////////////////////////////////////////////////////////////////////////////
// ustring
////////////////////////////////////////////////////////////////////////////////

ustring toUstr(int n) {
    std::string str = std::to_string(n);
    return ustring(str.begin(), str.end());
}

ustring toUstr(std::string s) {
    return reinterpret_cast<const unsigned char*>(s.c_str());
}

ustring toUstr(unsigned char* p, int len) {
    return ustring(p, len);
}

std::string fromUstr(ustring ustr) {
    std::string str;
    for (unsigned char c : ustr) {
        str += static_cast<char>(c);
    }
    return str;
}

std::ostream& operator <<(std::ostream& os, const ustring ustr) {
    return os << fromUstr(ustr);
}

////////////////////////////////////////////////////////////////////////////////
// IEncryptable
////////////////////////////////////////////////////////////////////////////////

template class IEncryptable<int>;

template <typename T>
IEncryptable<T>::IEncryptable(T val) {
    this->val = val;
}

template <typename T>
T IEncryptable<T>::get() {
    return this->val;
}

Id::Id(int val) : IEncryptable<int>(val) {}

ustring Id::toUstr() {
    return ::toUstr(this->val);
}

Id Id::fromUstr(ustring ustr) {
    std::string str(ustr.begin(), ustr.end());
    return Id(std::stoi(str));
}

Id abs(const Id& id) {
    return Id(abs(id.val));
}

void operator ++(Id& id, int _) {
    id.val++;
}

Id operator +(const Id& id1, const Id& id2) {
    return id1.val + id2.val;
}

Id operator +(const Id& id1, const int n) {
    return Id(id1.val + n);
}

Id operator -(const Id& id1, const Id& id2) {
    return id1.val - id2.val;
}

Id operator -(const Id& id1, const int n) {
    return Id(id1.val - n);
}

bool operator ==(const Id& id1, const Id& id2) {
    return id1.val == id2.val;
}

bool operator <(const Id& id1, const Id& id2) {
    return id1.val < id2.val;
}

bool operator >(const Id& id1, const Id& id2) {
    return id1.val > id2.val;
}

bool operator <=(const Id& id1, const Id& id2) {
    return id1.val <= id2.val;
}

bool operator >=(const Id& id1, const Id& id2) {
    return id1.val >= id2.val;
}

std::ostream& operator <<(std::ostream& os, const Id& id) {
    return os << id.val;
}

template <typename T>
ustring toUstr(IEncryptable<T>& iEncryptable) {
    return iEncryptable.toUstr();
}

template ustring toUstr(IEncryptable<Id>& id);

////////////////////////////////////////////////////////////////////////////////
// Range
////////////////////////////////////////////////////////////////////////////////

template class Range<Id>;
template class Range<Kw>;

template <typename T>
// can't call default constructor for `std::pair` without explicit vals? `0, 0` is supposed to be default
Range<T>::Range() : std::pair<T, T>(0, 0) {}

template <typename T>
Range<T>::Range(const T& start, const T& end) : std::pair<T, T>(start, end) {}

template <typename T>
Range<T> Range<T>::fromStr(std::string str) {
    std::regex re("(.*?)-(.*?)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 2) {
        std::cerr << "Error: bad string passed to `Range.Range()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }

    Range<T> range;
    range.first = T(std::stoi(matches[0].str()));
    range.second = T(std::stoi(matches[1].str()));
    return range;
}

template <typename T>
T Range<T>::size() {
    return (T)abs(this->second - this->first);
}

template <typename T>
bool Range<T>::contains(Range<T> range) {
    return this->first <= range.first && this->second >= range.second;
}

template <typename T>
bool Range<T>::isDisjointWith(Range<T> range) {
    return this->second < range.first || this->first > range.second;
}

template <typename T>
std::ostream& operator <<(std::ostream& os, const Range<T>& range) {
    return os << range.first << "-" << range.second;
}

template std::ostream& operator <<(std::ostream& os, const Range<Id>& range);
template std::ostream& operator <<(std::ostream& os, const Range<Kw>& range);

template<typename T>
std::string operator +(const std::string& str, const Range<T>& range) {
    std::stringstream ss;
    ss << str << range;
    return ss.str();
}

template std::string operator +(const std::string& str, const Range<Id>& range);
template std::string operator +(const std::string& str, const Range<Kw>& range);

template <typename T>
ustring toUstr(Range<T> range) {
    return toUstr(range.first) + toUstr("-") + toUstr(range.second);
}

template ustring toUstr(Range<Id> range);
template ustring toUstr(Range<Kw> range);

// todo temp?

template class IEncryptable<std::pair<KwRange, IdRange>>;

SrciDb1Doc::SrciDb1Doc(KwRange kwRange, IdRange idRange)
        : IEncryptable<std::pair<KwRange, IdRange>>(std::pair<KwRange, IdRange> {kwRange, idRange}) {}

ustring SrciDb1Doc::toUstr() {
    std::string str = "(" + this->val.first + "," + this->val.second + ")";
    return ::toUstr(str);
}

std::pair<KwRange, IdRange> SrciDb1Doc::fromUstr(ustring ustr) {
    std::string str = ::fromUstr(ustr);
    std::regex re("\\((.*?),(.*?)\\)");
    std::smatch matches;
    if (!std::regex_search(str, matches, re) || matches.size() != 2) {
        std::cerr << "Error: bad string passed to `SrciDb1Doc.fromUstr()`, the world is going to end" << std::endl;
        exit(EXIT_FAILURE);
    }
    KwRange kwRange = KwRange::fromStr(matches[0].str());
    IdRange idRange = IdRange::fromStr(matches[1].str());
    return std::pair<KwRange, IdRange> {kwRange, idRange};
}

template ustring toUstr(IEncryptable<SrciDb1Doc>& srciDb1Doc);

std::ostream& operator <<(std::ostream& os, const SrciDb1Doc& srciDb1Doc) {
    return os << "(" << srciDb1Doc.val.first << ", [" << srciDb1Doc.val.second << "])";
}

////////////////////////////////////////////////////////////////////////////////
// TDAG
////////////////////////////////////////////////////////////////////////////////

template class TdagNode<Kw>;
template class TdagNode<Id>;

template <typename T>
TdagNode<T>::TdagNode(const Range<T>& range) : range(range) {
    this->left = nullptr;
    this->right = nullptr;
    this->extraParent = nullptr;
}

template <typename T>
TdagNode<T>::TdagNode(TdagNode<T>* left, TdagNode<T>* right) {
    this->range = Range<T> {left->range.first, right->range.second};
    this->left = left;
    this->right = right;
    this->extraParent = nullptr;
}

template <typename T>
TdagNode<T>::~TdagNode() {
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
template <typename T>
std::list<TdagNode<T>*> TdagNode<T>::traverse() {
    std::unordered_set<TdagNode<T>*> nodes;
    return this->traverse(nodes);
}

template <typename T>
std::list<TdagNode<T>*> TdagNode<T>::traverse(std::unordered_set<TdagNode<T>*>& extraParents) {
    std::list<TdagNode<T>*> nodes;
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
template <typename T>
TdagNode<T>* TdagNode<T>::findSrc(Range<T> targetRange) {
    std::map<T, TdagNode<T>*> srcCandidates;
    auto addCandidate = [&](TdagNode<T>* node) {
        if (node == nullptr || !node->range.contains(targetRange)) {
            return T(-1);
        }

        T diff = (targetRange.first - node->range.first) + (node->range.second - targetRange.second);
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
    T diff = T(-1);
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
        TdagNode<T>* leftSrc = this->left->findSrc(targetRange);
        diff = addCandidate(leftSrc);
        if (diff == 0) {
            return leftSrc;
        }
    }
    if (this->right != nullptr) {
        TdagNode<T>* rightSrc = this->right->findSrc(targetRange);
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

template <typename T>
std::list<Range<T>> TdagNode<T>::traverseLeaves() {
    std::list<Range<T>> leafVals;
    std::list<TdagNode<T>*> nodes = this->traverse();
    for (TdagNode<T>* node : nodes) {
        if (node->left == nullptr && node->right == nullptr) {
            leafVals.push_back(node->range);
        }
    }
    return leafVals;
}

template <typename T>
std::list<TdagNode<T>*> TdagNode<T>::getLeafAncestors(Range<T> leafRange) {
    std::list<TdagNode<T>*> ancestors = {this};

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

template <typename T>
Range<T> TdagNode<T>::getRange() {
    return this->range;
}

template <typename T>
TdagNode<T>* TdagNode<T>::buildTdag(T maxLeafVal) {
    std::set<Range<T>> leafVals;
    for (T i = 0; i <= maxLeafVal; i++) {
        leafVals.insert(Range<T> {i, i});
    }
    return TdagNode<T>::buildTdag(leafVals);
}

template <typename T>
TdagNode<T>* TdagNode<T>::buildTdag(std::set<Range<T>> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `TdagNode.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // list to hold nodes while building; initialize with leaves
    std::list<TdagNode<T>*> l;
    for (Range<T> leafVal : leafVals) {
        l.push_back(new TdagNode<T>(leafVal));
    }

    // build full binary tree from leaves (this is my own algorithm i have no idea how good it is)
    // trees seem balanced though which is nice
    while (l.size() > 1) {
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        TdagNode<T>* node1 = l.front();
        l.pop_front();
        // find a contiguous node
        for (auto it = l.begin(); it != l.end(); it++) {
            TdagNode<T>* node2 = *it;
            if (node2->range.first - 1 == node1->range.second) {
                // if `node1` is the left child of new parent node
                TdagNode<T>* parent = new TdagNode<T>(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->range.second + 1 == node1->range.first) {
                // if `node2` is the left child of new parent node
                TdagNode<T>* parent = new TdagNode<T>(node2, node1);
                l.push_back(parent);
                l.erase(it);
                break;
            }
        }
    }

    // add extra TDAG nodes
    TdagNode<T>* tdag = l.front();
    std::list<TdagNode<T>*> nodes = tdag->traverse();
    while (!nodes.empty()) {
        TdagNode<T>* node = nodes.front();
        nodes.pop_front();
        if (node->left == nullptr
                || node->right == nullptr
                || node->left->right == nullptr
                || node->right->left == nullptr) {
            continue;
        }

        TdagNode<T>* extraParent = new TdagNode<T>(node->left->right, node->right->left);
        node->left->right->extraParent = extraParent;
        node->right->left->extraParent = extraParent;
        // using my method of finding places to add extra nodes, extra nodes themselves must also be checked
        nodes.push_back(extraParent);
    }

    return tdag;
}

template <typename T>
std::ostream& operator <<(std::ostream& os, TdagNode<T>* node) {
    std::list<TdagNode<T>*> nodes = node->traverse();
    for (TdagNode<T>* node : nodes) {
        os << node->getRange() << std::endl;
    }
    return os;
}

template std::ostream& operator <<(std::ostream& os, TdagNode<Id>* node);
template std::ostream& operator <<(std::ostream& os, TdagNode<Kw>* node);

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
