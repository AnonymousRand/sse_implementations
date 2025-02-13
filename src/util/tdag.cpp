#include "tdag.h"

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
TdagNode<T>* TdagNode<T>::findSrcRecur(const Range<T>& targetRange) {
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
TdagNode<T>* TdagNode<T>::findSrc(const Range<T>& targetRange) {
    TdagNode<T>* src = this->findSrcRecur(targetRange);
    if (src == nullptr) {
        return this;
    }
    return src;
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
std::list<TdagNode<T>*> TdagNode<T>::getLeafAncestors(const Range<T>& leafRange) {
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
Range<T> TdagNode<T>::getRange() const {
    return this->range;
}

template <typename T>
TdagNode<T>* TdagNode<T>::buildTdag(T& maxLeafVal) {
    std::set<Range<T>> leafVals;
    for (T i = 0; i <= maxLeafVal; i++) {
        leafVals.insert(Range<T> {i, i});
    }
    return TdagNode<T>::buildTdag(leafVals);
}

template <typename T>
TdagNode<T>* TdagNode<T>::buildTdag(std::set<Range<T>>& leafVals) {
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

////////////////////////////////////////////////////////////////////////////////
// Template Instantiations
////////////////////////////////////////////////////////////////////////////////

template class TdagNode<Kw>;
template class TdagNode<Id>;

template std::ostream& operator <<(std::ostream& os, TdagNode<Id>* node);
template std::ostream& operator <<(std::ostream& os, TdagNode<Kw>* node);
