#include "tdag.h"

TdagNode::TdagNode(KwRange kwRange) {
    this->kwRange = kwRange;
    this->left = nullptr;
    this->right = nullptr;
    this->extraParent = nullptr;
}

TdagNode::TdagNode(TdagNode* left, TdagNode* right) {
    this->kwRange = KwRange {left->kwRange.first, right->kwRange.second};
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

// current algo uses divide-and-conquer to find best SRC
// which is worst-case O(N) for N nodes instead of O(log R) as described in paper
// todo evaluate new early exit speed
TdagNode* TdagNode::findSrc(KwRange targetKwRange) {
    std::map<int, TdagNode*> srcCandidates;

    auto addCandidate = [&](TdagNode* node) {
        if (node == nullptr || !isContainingRange(node->kwRange, targetKwRange)) {
            return -1;
        }

        int diff = (targetKwRange.first - node->kwRange.first) + (node->kwRange.second - targetKwRange.second);
        srcCandidates[diff] = node;
        return diff;
    };

    // find best SRC between current node, best SRC in left subtree, best SRC in right subtree, and extra TDAG parent 
    int diff = -1;
    if (this->extraParent != nullptr) {
        diff = addCandidate(this->extraParent);
        if (diff == 0) {
            return this->extraParent;
        }
    }
    // if the extra TDAG parent is not a candidate and the current node's range is narrower than the target range,
    // we wont't have to go further down the tree
    // todo verify
    if (diff == -1 && rangeSize(this->kwRange) < rangeSize(targetKwRange)) {
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

    return srcCandidates.begin()->second; // take advantage of the fact that `std::map`s are sorted by key
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

    if (this->left != nullptr && isContainingRange(this->left->kwRange, leafKwRange)) {
        ancestors.splice(ancestors.end(), this->left->getLeafAncestors(leafKwRange));
    }
    if (this->right != nullptr && isContainingRange(this->right->kwRange, leafKwRange)) {
        ancestors.splice(ancestors.end(), this->right->getLeafAncestors(leafKwRange));
    }
    if (this->extraParent != nullptr && isContainingRange(this->extraParent->kwRange, leafKwRange)) {
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
        leafVals.insert(KwRange {i, i});
    }
    return TdagNode::buildTdag(leafVals);
}

TdagNode* TdagNode::buildTdag(std::set<KwRange> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `TdagNode.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // list to hold nodes while building (linked list chosen for efficiency)
    // initially load just the leaves as nodes into list
    std::list<TdagNode*> l;
    for (KwRange leafVal : leafVals) {
        l.push_back(new TdagNode(leafVal));
    }

    // build full binary tree from leaves (this is my own algorithm i have no idea how good it is)
    // the trees seem pretty balanced though which is nice
    // with every iteration of the outer while loop, the list size shrinks by 1 as two nodes are combined into one
    // thus we have O(n) for n leaves, assuming few iterations of inner for loop (which seems to be true experimentally)
    while (l.size() > 1) {
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        TdagNode* node1 = l.front();
        l.pop_front();
        // find a contiguous node
        for (auto it = l.begin(); it != l.end(); it++) {
            TdagNode* node2 = *it;
            if (node2->kwRange.first - 1 == node1->kwRange.second) {
                // if `node1` is the left child of new parent node
                TdagNode* parent = new TdagNode(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->kwRange.second + 1 == node1->kwRange.first) {
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
        if (node->left == nullptr || node->right == nullptr
                || node->left->right == nullptr || node->right->left == nullptr) {
            continue;
        }

        TdagNode* extraParent = new TdagNode(node->left->right, node->right->left);
        node->left->right->extraParent = extraParent;
        node->right->left->extraParent = extraParent;
        // using the method I have for finding spots to add extra nodes, extra nodes themselves must also be checked
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
