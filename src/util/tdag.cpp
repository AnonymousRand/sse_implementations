#include <cstdlib>
#include <deque>

#include "tdag.h"


template <class T>
TdagNode<T>::TdagNode(const Range<T>& range) : range(range), left(nullptr), right(nullptr), extraParent(nullptr) {}


template <class T>
TdagNode<T>::TdagNode(TdagNode<T>* left, TdagNode<T>* right) :
        range(Range<T> {left->range.first, right->range.second}), left(left), right(right), extraParent(nullptr) {}


template <class T>
TdagNode<T>::TdagNode(T maxLeafVal) {
    std::set<Range<T>> leafVals; // `set` automatically sorts leaf values in ascending order
    for (T i = T(DB_KW_MIN); i <= maxLeafVal; i++) {
        leafVals.insert(Range<T> {i, i});
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
            std::cout << "im sorry what" << std::endl;
            std::exit(EXIT_FAILURE);
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
        extraParent->isExtraParent = true;
        // using my method of finding places to add extra nodes, extra nodes themselves must also be checked
        nodes.push_back(extraParent);
    }

    *this = *tdag;
}


template <class T>
TdagNode<T>::~TdagNode() {
    // prevent infinite `delete` recursion where extra parents go back to their children
    // which go back to their extra parents and so on
    if (this->isExtraParent) {
        return;
    }
    if (this->left != nullptr) {
        delete this->left;
        this->left = nullptr;
    }
    if (this->right != nullptr) {
        delete this->right;
        this->right = nullptr;
    }
    if (this->extraParent != nullptr) {
        // prevent double frees (since two nodes have the same `extraParent`) by setting the other such node's
        // `extraParent` to nullptr, indicating it has been (or is about to be, I guess) freed
        if (this == this->extraParent->left) {
            this->extraParent->right->extraParent = nullptr;
        } else if (this == this->extraParent->right) {
            this->extraParent->left->extraParent = nullptr;
        }
        delete this->extraParent;
        this->extraParent = nullptr;
    }
}


// DFS preorder but with additional traversal of TDAG's extra parent nodes
// track `extraParent` nodes in an `unordered_set` to prevent duplicates
template <class T>
std::list<TdagNode<T>*> TdagNode<T>::traverse() {
    std::unordered_set<TdagNode<T>*> extraParents;
    return this->traverseHelper(extraParents);
}


template <class T>
std::list<TdagNode<T>*> TdagNode<T>::traverseHelper(std::unordered_set<TdagNode<T>*>& extraParents) {
    std::list<TdagNode<T>*> nodes;
    nodes.push_front(this);

    // `list` returned so this splicing is fast
    if (this->left != nullptr) {
        nodes.splice(nodes.end(), this->left->traverseHelper(extraParents));
    }
    if (this->right != nullptr) {
        nodes.splice(nodes.end(), this->right->traverseHelper(extraParents));
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
template <class T>
Range<T> TdagNode<T>::findSrc(const Range<T>& targetRange) {
    // if the current node is disjoint with the target range, it is impossible for
    // its children or extra TDAG parent to be the SRC, so we can early exit
    if (this->range.isDisjointFrom(targetRange)) {
        return DUMMY_RANGE<T>();
    }

    // else find best SRC between current node, best SRC in left subtree, best SRC in right subtree,
    // and extra TDAG parent 
    std::map<T, Range<T>> candidates;
    auto tryAddCandidate = [&](Range<T> range) {
        if (range == DUMMY_RANGE<T>() || !range.contains(targetRange)) {
            return T(-1);
        }

        T diff = (targetRange.first - range.first) + (range.second - targetRange.second);
        candidates[diff] = range;
        return diff;
    };

    T diff = T(-1);
    if (this->extraParent != nullptr) {
        Range<T> extraParentRange = this->extraParent->range;
        diff = tryAddCandidate(extraParentRange);
        if (diff == 0) {
            return extraParentRange;
        }
    }
    // if the current node's range is narrower than the target range, it is impossible for
    // its children to be the SRC, so we only have to test its extra TDAG parent
    if (this->range.size() < targetRange.size()) {
        // if the earlier `if` case concluded that `extraParent` is not a valid cover
        if (diff == -1) {
            return DUMMY_RANGE<T>();
        }
        // else if already seen that `extraParent` is valid (just not a perfect cover)
        return this->extraParent->range;
    }

    diff = tryAddCandidate(this->range);
    if (diff == 0) {
        return this->range;
    }
    if (this->left != nullptr) {
        Range<T> leftSrc = this->left->findSrc(targetRange);
        diff = tryAddCandidate(leftSrc);
        if (diff == 0) {
            return leftSrc;
        }
    }
    if (this->right != nullptr) {
        Range<T> rightSrc = this->right->findSrc(targetRange);
        diff = tryAddCandidate(rightSrc);
        if (diff == 0) {
            return rightSrc;
        }
    }

    if (candidates.empty()) {
        return DUMMY_RANGE<T>();
    }
    return candidates.begin()->second; // take advantage of `std::map`s being sorted by key
}


template <class T>
std::list<Range<T>> TdagNode<T>::getLeafAncestors(const Range<T>& target) {
    std::list<Range<T>> ancestors {this->range};

    if (this->left != nullptr && this->left->range.contains(target)) {
        ancestors.splice(ancestors.end(), this->left->getLeafAncestors(target));
    }
    if (this->right != nullptr && this->right->range.contains(target)) {
        ancestors.splice(ancestors.end(), this->right->getLeafAncestors(target));
    }
    if (this->extraParent != nullptr && this->extraParent->range.contains(target)) {
        ancestors.push_back(this->extraParent->range);
    }

    return ancestors;
}


template <class T>
std::ostream& operator <<(std::ostream& os, TdagNode<T>* node) {
    std::list<TdagNode<T>*> nodes = node->traverse();
    for (TdagNode<T>* node : nodes) {
        os << node->range << std::endl;
    }
    return os;
}


template class TdagNode<Kw>;
//template class TdagNode<IdAlias>;

template std::ostream& operator <<(std::ostream& os, TdagNode<Kw>* node);
//template std::ostream& operator <<(std::ostream& os, TdagNode<IdAlias>* node);
