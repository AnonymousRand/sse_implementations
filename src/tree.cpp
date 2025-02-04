#include "tree.h"

#include <iostream>
#include <list>
#include <stdlib.h>
#include <string>

Node::Node(int startVal, int endVal) {
    this->startVal = startVal;
    this->endVal = endVal;
    this->left = nullptr;
    this->right = nullptr;
}

Node::Node(Node* left, Node* right) {
    this->startVal = left->startVal;
    this->endVal = right->endVal;
    this->left = left;
    this->right = right;
}

Node* Node::buildFromLeafVals(std::vector<int> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `Node.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // linked list used to hold nodes while building (linked list chosen for efficiency)
    std::list<Node*> l;
    // initially load just the leaves into list
    for (int leafVal : leafVals) {
        l.push_back(new Node(leafVal, leafVal));
    }

    // build full binary tree from leaves
    // (this is my own algorithm i have no idea how good it is)
    while (l.size() > 1) {
        // todo temp
        std::cout << "---------\n";
        for (auto v : l) {
            std::cout << v->startVal << " - " << v->endVal << "\n";
        }
        // find first two nodes from `l` that have contiguous ranges and join them with a parent node
        // then delete these two nodes and append their new parent node to `l` to keep building tree
        Node* node1 = l.front();
        l.pop_front();
        for (auto it = l.begin(); it != l.end(); it++) {
            Node* node2 = *it;
            if (node2->startVal - 1 == node1->endVal) {
                // if `node1` is the left child of new parent node
                Node* parent = new Node(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->endVal + 1 == node1->startVal) {
                // if `node2` is the left child of new parent node
                Node* parent = new Node(node2, node1);
                l.push_back(parent);
                l.erase(it);
                break;
            }
        }
    }

    return l.front();
}

Node::~Node() {
    if (this->left != nullptr) {
        delete this->left;
    }
    if (this->right != nullptr) {
        delete this->right;
    }
}

std::forward_list<Node*> Node::traverse(Node* root) {
    std::foward_list<Node*> nodes;
    if (root == nullptr) {
        return nodes;
    }

    nodes.push_front(root);
    // todo move if nullptr check here?
    nodes.splice_after(x.cbegin(), traverse(root->left));
    nodes.splice_after(x.cbegin(), traverse(root->right));
    return nodes;
}

std::ostream& operator << (std::ostream& os, const Node& node) {
    // todo
    return os;
}

////////////////////////////////////////////////////////////////////////////////

TdagNode* TdagNode::buildFromLeafVals(std::vector<int> leafVals) {
    // build base tree
    Node* baseTree = Node::buildFromLeafVals(leafVals);
    TdagNode* tdag = TdagNode(baseTree->left, baseTree->right);

    // preliminary idea: use DFS (private method `traverse()` also used by getSubtreeLeafVals()) to return list of nodes
    // then on every node:
    /*
        if (node->left == nullptr || node->right == nullptr) {
            continue;
        }
        if (node->left->right == nullptr || node->right->left == nullptr) {
            continue;
        }
        Node* intermediate = new Node(n->left->right, n->right->left);
    */
    // however this probably requires DFS to be modified to support going through to parent nodes
    // as these intermediate nodes are not ever along a branch from the root
}
