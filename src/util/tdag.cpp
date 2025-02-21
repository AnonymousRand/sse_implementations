#include <deque>
#include <map>

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
    std::unordered_set<TdagNode<T>*> extraParents;
    return this->traverse(extraParents);
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

// basically traverses tree with DFS and early exits to find best SRC
template <typename T>
TdagNode<T>* TdagNode<T>::findSrc(const Range<T>& targetRange) {
    // if the current node is disjoint with the target range, it is impossible for
    // its children or extra TDAG parent to be the SRC, so we can early exit
    if (this->range.isDisjointWith(targetRange)) {
        return nullptr;
    }

    // else find best SRC between current node, best SRC in left subtree, best SRC in right subtree,
    // and extra TDAG parent 
    std::map<T, TdagNode<T>*> candidates;
    auto tryToAddCandidate = [&](TdagNode<T>* node) {
        if (node == nullptr || !node->range.contains(targetRange)) {
            return T(-1);
        }

        T diff = (targetRange.first - node->range.first) + (node->range.second - targetRange.second);
        candidates[diff] = node;
        return diff;
    };

    T diff = T(-1);
    if (this->extraParent != nullptr) {
        diff = tryToAddCandidate(this->extraParent);
        if (diff == 0) {
            return this->extraParent;
        }
    }
    // if the current node's range is narrower than the target range, it is impossible for
    // its children to be the SRC, so we only have to test its extra TDAG parent
    if (this->range.size() < targetRange.size()) {
        if (diff == -1) {
            return nullptr;
        }
        return this->extraParent;
    }

    diff = tryToAddCandidate(this);
    if (diff == 0) {
        return this;
    }
    if (this->left != nullptr) {
        TdagNode<T>* leftSrc = this->left->findSrc(targetRange);
        diff = tryToAddCandidate(leftSrc);
        if (diff == 0) {
            return leftSrc;
        }
    }
    if (this->right != nullptr) {
        TdagNode<T>* rightSrc = this->right->findSrc(targetRange);
        diff = tryToAddCandidate(rightSrc);
        if (diff == 0) {
            return rightSrc;
        }
    }

    if (candidates.empty()) {
        return nullptr;
    }
    return candidates.begin()->second; // take advantage of `std::map`s being sorted by key
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
    std::list<TdagNode<T>*> ancestors {this};

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
    for (T i = T(0); i <= maxLeafVal; i++) {
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

    // array to hold nodes while building; initialize with leaves
    // `deque` seems to perform marginally better than `list` or `vector` and seems to be the most natural choice here
    std::deque<TdagNode<T>*> l;
    for (Range<T> leafVal : leafVals) {
        l.push_back(new TdagNode<T>(leafVal));
    }

    // build full binary tree from leaves (this is my own algorithm i have no idea how good it is)
    // trees are balanced though which is nice
    auto joinNodes = [&](TdagNode<T>* node1, auto it) {
        TdagNode<T>* node2 = *it;
        if (node2->range.first - 1 == node1->range.second) {
            // if `node1` is the left child of new parent node
            TdagNode<T>* parent = new TdagNode<T>(node1, node2);
            l.erase(it); // have to `erase()` before `push_back()` to avoid messy memory issues
            l.push_back(parent);
            return true;
        }
        if (node2->range.second + 1 == node1->range.first) {
            // if `node2` is the left child of new parent node
            TdagNode<T>* parent = new TdagNode<T>(node2, node1);
            l.erase(it);
            l.push_back(parent);
            return true;
        }
        return false;
    };

    while (l.size() > 1) {
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        TdagNode<T>* node1 = l.front();
        l.pop_front();
        // find a contiguous node; I proved that this must either be the next node at the front, or the one at the back
        if (joinNodes(node1, l.begin())) {
            continue;
        } 
        if (!joinNodes(node1, l.end() - 1)) {
            std::cout << "nani" << std::endl;
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

template class TdagNode<Id>;
template class TdagNode<Kw>;

template std::ostream& operator <<(std::ostream& os, TdagNode<Id>* node);
template std::ostream& operator <<(std::ostream& os, TdagNode<Kw>* node);
