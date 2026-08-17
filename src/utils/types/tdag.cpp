#include "utils/types/tdag.h"

#include <concepts>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <list>
#include <unordered_set>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//==============================================================================
// `TdagNode`
//==============================================================================


template <std::integral T>
TdagNode<T>::TdagNode(TdagNode<T>* left, TdagNode<T>* right) :
    range(Range<T> {left->range.first, right->range.second}),
    left(left), right(right), extraParent(nullptr) {}


template <std::integral T>
TdagNode<T>::TdagNode(const Range<T>& leafRange) {
    if (leafRange.size() < 1) {
        return;
    }

    // if leaf node
    if (leafRange.size() == 1) {
        this->range = leafRange;
        this->left = nullptr;
        this->right = nullptr;
        this->extraParent = nullptr;
        return;
    }

    std::vector<Range<T>> leafs;
    leafs.reserve(leafRange.size());
    for (T i = leafRange.first; i <= leafRange.second; i++) {
        leafs.push_back(Range<T> {i, i});
    }

    // array to hold nodes while building; initialize with leaves
    // (`deque` seems to perform marginally better than `list` or `vector` and seems to be
    // the most natural choice here)
    std::deque<TdagNode<T>*> l;
    for (const Range<T>& leaf : leafs) {
        l.push_back(new TdagNode<T>(leaf));
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
        // find first two nodes from `l` that have contiguous ranges and join them with
        // a parent node, then delete these two nodes and append their new parent node
        // to `l` to keep building the tree
        TdagNode<T>* node1 = l.front();
        l.pop_front();
        // find a contiguous node (I've proved that this must either be the next node
        // at the front, or the one at the back)
        if (joinNodes(node1, l.begin())) {
            continue;
        } 
        if (!joinNodes(node1, l.end() - 1)) {
            std::cerr << "Error: TdagNode::TdagNode(): im sorry what" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // add extra TDAG nodes
    TdagNode<T>* tdag = l.front();
    std::list<const TdagNode<T>*> nodes = tdag->traverse();
    while (!nodes.empty()) {
        const TdagNode<T>* node = nodes.front();
        nodes.pop_front();
        if (node->left == nullptr
            || node->right == nullptr
            || node->left->right == nullptr
            || node->right->left == nullptr)
        {
            continue;
        }

        TdagNode<T>* extraParent = new TdagNode<T>(node->left->right, node->right->left);
        node->left->right->extraParent = extraParent;
        node->right->left->extraParent = extraParent;
        extraParent->isExtraParent = true;
        // using my method of finding places to add extra nodes, extra nodes themselves
        // must also be checked
        nodes.push_back(extraParent);
    }

    *this = *tdag;
}


template <std::integral T>
TdagNode<T>::TdagNode(T leafRangeStart, T leafRangeEnd) :
    TdagNode<T>(Range {leafRangeStart, leafRangeEnd}) {}


template <std::integral T>
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
        // prevent double frees (since two nodes have the same `extraParent`) by
        // setting the other such node's `extraParent` to nullptr, indicating it
        // has been (or is about to be, I guess) freed
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
template <std::integral T>
std::list<const TdagNode<T>*> TdagNode<T>::traverse() const {
    std::unordered_set<TdagNode<T>*> extraParents;
    return this->traverseHelper(extraParents);
}


template <std::integral T>
std::list<const TdagNode<T>*> TdagNode<T>::traverseHelper(
    std::unordered_set<TdagNode<T>*>& extraParents
) const {
    std::list<const TdagNode<T>*> nodes;
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
        // if insertion succeeded; i.e., the node is not already in the `unordered_set`
        // (this prevents duplicates)
        if (res.second) {
            nodes.push_back(this->extraParent);
        }
    }

    return nodes;
}


template <std::integral T>
Range<T> TdagNode<T>::findSrc(Range<T> targetRange) const {
    // if target range exceeds this entire tree's range on either side, return what we can
    if (targetRange.first < this->range.first) {
        targetRange.first = this->range.first;
    }
    if (targetRange.second > this->range.second) {
        targetRange.second = this->range.second;
    }
    return this->findSrcHelper(targetRange);
}


// basically traverses tree with DFS and early exits to find best SRC
template <std::integral T>
Range<T> TdagNode<T>::findSrcHelper(const Range<T>& targetRange) const {
    // if the current node is disjoint with the target range, it is impossible for
    // its children or extra TDAG parent to be the SRC, so we can early exit
    if (this->range.isDisjointFrom(targetRange)) {
        return Range<T>::DUMMY();
    }

    // else find best SRC between current node, best SRC in left subtree, best SRC in right subtree,
    // and extra TDAG parent 
    std::map<T, Range<T>> candidates;
    auto tryAddCandidate = [&](Range<T> range) {
        if (range == Range<T>::DUMMY() || !range.contains(targetRange)) {
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
    // its children to be the SRC, so its extra TDAG parent is the only possible SRC candidate
    if (this->range.size() < targetRange.size()) {
        // if the earlier `if` case concluded that `extraParent` is not a valid cover,
        // return nothing
        if (diff == -1) {
            return Range<T>::DUMMY();
        }
        // else if already seen that `extraParent` is valid (just not a perfect cover), return it
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
        return Range<T>::DUMMY();
    }
    return candidates.begin()->second; // take advantage of `std::map`s being sorted by key
}


template <std::integral T>
std::list<Range<T>> TdagNode<T>::getLeafAncestors(const Range<T>& target) const {
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


template <std::integral T>
std::ostream& operator <<(std::ostream& os, TdagNode<T>* node) {
    std::list<const TdagNode<T>*> nodes = node->traverse();
    for (const TdagNode<T>* node : nodes) {
        os << node->range << std::endl;
    }
    return os;
}


//------------------------------------------------------------------------------
// explicit template instantiations


template class TdagNode<Kw>;
//template class TdagNode<IdAlias>;

template std::ostream& operator <<(std::ostream& os, TdagNode<Kw>* node);
//template std::ostream& operator <<(std::ostream& os, TdagNode<IdAlias>* node);


//==============================================================================
// utils
//==============================================================================


namespace utils::tdag {


bigint calcTdagTupleCount(bigint leafCount) {
    // a formula for the total number of tuples above level i, where n = leaf count
    // and m = log_2(n) is the level number of the top level (where bottom level is 0)
    // is (m - i)(2n) - (1 - 2^(i-m)) 2^(m+1):
    // the bucket count at level j (j >= 1) is 2^(1-j)n - 1, and the bucket size at level j is 2^j,
    // so the total number of items above level i is
    //   (2^(1-m)n - 1)2^m  +  (2^(1-(m-1))n - 1)2^(m-1)  +  ...  +  (2^(1-(i+1))n - 1)2^(i+1)
    //           ^                         ^                                     ^
    //    items in level m         items in level m-1                    items in level i+1
    //
    // = (2n - 2^m) + (2n - 2^(m-1)) + ... + (2n - 2^(i+1))   (note there are m - i terms)
    // = (m - i)(2n) - (2^m + 2^(m-1) + ... + 2^(i+1))
    // = (m - i)(2n) - ((1 - 0.5^(m-i)) 2^m / 0.5)            (sum of geometric series)
    // = (m - i)(2n) - (1 - 2^(i-m)) 2^(m+1)
    //
    // so here we are just calculating the case where i is 0 (so number of tuples
    // above the bottommost level) and then adding the number of leaves
    bigint topLevelNum = std::log2(leafCount);
    return topLevelNum * (2 * leafCount)
        - (1 - std::pow(2, -topLevelNum)) * std::pow(2, topLevelNum + 1)
        + leafCount;
}


template <IsDbTuple DbTuple>
void buildTdagAndDbFromLeaves(
    TdagNode<typename DbTuple::DbKwType>*& tdag, Db<DbTuple>& db, bool shouldPadDb
) {
    using DbKw = typename DbTuple::DbKwType;

    // obtain TDAG leaf bounds
    Range<DbKw> dbKwBounds = db.findDbKwBounds();
    DbKw maxDbKw = dbKwBounds.second;

    // pad if necessary
    if (shouldPadDb) {
        db.pad(maxDbKw);
    }

    // construct TDAG
    tdag = new TdagNode<DbKw>(dbKwBounds.first, maxDbKw);
    
    // replicate every (leaf) DB tuple to all TDAG nodes that cover it
    replDbForTdag<DbTuple>(db, tdag);
}


template <IsDbTuple DbTuple>
void replDbForTdag(Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag) {
    using DbKw = typename DbTuple::DbKwType;

    bigint dbSize = db.size();
    db.reserve(dbSize + calcTdagTupleCount(dbSize));
    for (bigint i = 0; i < dbSize; i++) {
        DbTuple tuple = db[i];
        Range<DbKw> dbKwRange = tuple.getDbKwRange();
        std::list<Range<DbKw>> ancestors = tdag->getLeafAncestors(dbKwRange);
        for (const Range<DbKw>& ancestor : ancestors) {
            if (ancestor == dbKwRange) {
                continue;
            }
            DbTuple newTuple(tuple.getDbDoc(), ancestor);
            db.push_back(newTuple);
        }
    }
}


//------------------------------------------------------------------------------
// explicit template instantiations


template void buildTdagAndDb<Tuple<>>(
    TdagNode<Kw>*& tdag, Db<Tuple<>>& db, bool shouldPadDb
);
template void buildTdagAndDb<SrcIDb1Tuple>(
    TdagNode<Kw>*& tdag, Db<SrcIDb1Tuple>& db, bool shouldPadDb
);
//template void buildTdagAndDb<Tuple<IdAlias>>(
//    TdagNode<IdAlias>*& tdag, Db<Tuple<IdAlias>>& db, bool shouldPadDb
//);


template void replDbForTdag<Tuple<>>(Db<Tuple<>>& db, const TdagNode<Kw>* tdag);
template void replDbForTdag<SrcIDb1Tuple>(Db<SrcIDb1Tuple>& db, const TdagNode<Kw>* tdag);
//template void replDbForTdag<Tuple<IdAlias>>(
//    Db<Tuple<IdAlias>>& db, const TdagNode<IdAlias>* tdag
//);


} // namespace `utils::tdag`
