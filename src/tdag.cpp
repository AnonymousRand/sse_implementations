#include "tdag.h"

#include <iostream>
#include <map>
#include <list>
#include <stdlib.h>
#include <string>


// DFS but with additional traversal of TDAG's extra parent nodes
std::forward_list<TdagNode*> TdagNode::traverse() {
    std::forward_list<TdagNode*> nodes;
    nodes.push_front(this);
    if (this->left != nullptr) nodes.splice_after(nodes.cbegin(), this->left->traverse());
    if (this->right != nullptr) nodes.splice_after(nodes.cbegin(), this->right->traverse());
    if (this->extraParent != nullptr) nodes.push_front(this->extraParent);
    return nodes;
}


TdagNode::TdagNode(int startVal, int endVal) {
    this->startVal = startVal;
    this->endVal = endVal;
    this->left = nullptr;
    this->right = nullptr;
    this->extraParent = nullptr;
}


TdagNode::TdagNode(TdagNode* left, TdagNode* right) {
    this->startVal = left->startVal;
    this->endVal = right->endVal;
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


// current algo uses divide-and-conquer with early exits to find best SRC
// which is worst-case O(N) for N nodes instead of O(log R) as described in paper
// experimentally, this actually seems like O(log N), probably because of early exits
TdagNode* TdagNode::findSrc(std::tuple<int, int> range) {
    int rangeStart = std::get<0>(range);  // this syntax is very cursed
    int rangeEnd = std::get<1>(range);
    std::map<int, TdagNode*> srcCandidates;
    auto findDiff = [=](TdagNode* node) { // nested lambda function for code reuse
        return (rangeStart - node->startVal) + (node->endVal - rangeEnd);
    };

    // early exit and backtrack up the tree if current node doesn't encompass at least the entire range
    if (this->startVal > rangeStart || this->endVal < rangeEnd) {
        return nullptr;
    }

    // find best SRC between current node, best SRC in left subtree, and best SRC in right subtree
    int thisDiff = findDiff(this);
    if (thisDiff == 0) {
        return this;
    }
    srcCandidates[thisDiff] = this;
    if (this->left != nullptr) {
        TdagNode* leftSrc = this->left->findSrc(range);
        if (leftSrc != nullptr) {
            int leftDiff = findDiff(leftSrc);
            if (leftDiff == 0) {
                return leftSrc;
            }
            srcCandidates[leftDiff] = leftSrc;
        }
    }
    if (this->right != nullptr) {
        TdagNode* rightSrc = this->right->findSrc(range);
        if (rightSrc != nullptr) {
            int rightDiff = findDiff(rightSrc);
            if (rightDiff == 0) {
                return rightSrc;
            }
            srcCandidates[rightDiff] = rightSrc;
        }
    }

    return srcCandidates.begin()->second; // take advantage of the fact that `std::map`s are sorted by key
}


std::vector<int> TdagNode::traverseSrc() {
    std::vector<int> leafVals;
    std::forward_list<TdagNode*> nodes = this->traverse();
    for (TdagNode* node : nodes) {
        if (node->startVal == node->endVal) {
            leafVals.push_back(node->startVal);
        }
    }
    return leafVals;
}


TdagNode* TdagNode::buildTdag(std::vector<int> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `TdagNode.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // list to hold nodes while building (linked list chosen for efficiency)
    // initially load just the leaves as nodes into list
    std::list<TdagNode*> l;
    for (int leafVal : leafVals) {
        l.push_back(new TdagNode(leafVal, leafVal));
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
            if (node2->startVal - 1 == node1->endVal) {
                // if `node1` is the left child of new parent node
                TdagNode* parent = new TdagNode(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->endVal + 1 == node1->startVal) {
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
    std::forward_list<TdagNode*> nodes = tdag->traverse();
    while (!nodes.empty()) {
        TdagNode* node = nodes.front();
        nodes.pop_front();
        if (node->left == nullptr || node->right == nullptr) continue;
        if (node->left->right == nullptr || node->right->left == nullptr) continue;

        TdagNode* extraParent = new TdagNode(node->left->right, node->right->left);
        // IMPORTANT: only assign `extraParent` to its left child
        // so we don't have to keep track of visited nodes for traversal
        node->left->right->extraParent = extraParent;
        // using the method I have for finding spots to add extra nodes, extra nodes themselves must also be checked
        nodes.push_front(extraParent);
    }

    return tdag;
}


std::ostream& operator << (std::ostream& os, const TdagNode* node) {
    return os << node->startVal << " - " << node->endVal;
}
