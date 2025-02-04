#include "tdag.h"

#include <iostream>
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


TdagNode* TdagNode::findSrc(std::tuple<int, int> range) {
    int rangeStart = std::get<0>(range); // this syntax is very cursed
    int rangeEnd = std::get<1>(range);
    TdagNode* closestNode = nullptr;
    int diffOfClosestNode = -1;

    std::forward_list<TdagNode*> nodes = this->traverse();
    for (TdagNode* node : nodes) {
        // eliminate nodes that don't cover at least the whole range
        if (node->startVal > rangeStart || node->endVal < rangeEnd) {
            continue;
        }
        // compute how tight this node is to the range; the tightest node is the SRC
        int diffOfNode = (rangeStart - node->startVal) + (node->endVal - rangeEnd);
        if (diffOfNode < diffOfClosestNode || diffOfClosestNode == -1) {
            closestNode = node;
            diffOfClosestNode = diffOfNode;
        }
        // if we've found an exact cover, return instead of continuing to check all nodes
        if (diffOfNode == 0) {
            return node;
        }
    }
    return closestNode;
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

    // linked list used to hold nodes while building (linked list chosen for efficiency)
    std::list<TdagNode*> l;
    // initially load just the leaves into list
    for (int leafVal : leafVals) {
        l.push_back(new TdagNode(leafVal, leafVal));
    }

    // build full binary tree from leaves
    // (this is my own algorithm i have no idea how good it is)
    while (l.size() > 1) {
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        TdagNode* tdagNode1 = l.front();
        l.pop_front();
        for (auto it = l.begin(); it != l.end(); it++) {
            TdagNode* tdagNode2 = *it;
            if (tdagNode2->startVal - 1 == tdagNode1->endVal) {
                // if `tdagNode1` is the left child of new parent node
                TdagNode* parent = new TdagNode(tdagNode1, tdagNode2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (tdagNode2->endVal + 1 == tdagNode1->startVal) {
                // if `tdagNode2` is the left child of new parent node
                TdagNode* parent = new TdagNode(tdagNode2, tdagNode1);
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
        // using the method I have for finding spots to inject extra nodes, extra nodes themselves must also be checked
        nodes.push_front(extraParent);
    }

    return tdag;
}


std::ostream& operator << (std::ostream& os, const TdagNode* node) {
    return os << node->startVal << " - " << node->endVal;
}
