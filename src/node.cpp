#include "node.h"

#include <iostream>
#include <list>
#include <stdlib.h>
#include <string>

Node::Node(int startVal, int endVal) {
    this->startVal = startVal;
    this->endVal = endVal;
    this->left = NULL;
    this->right = NULL;
}

Node::Node(Node* left, Node* right) {
    this->startVal = left->startVal;
    this->endVal = right->endVal;
    this->left = left;
    this->right = right;
}

Node::~Node() {
    if (this->left != NULL) {
        delete this->left;
    }
    if (this->right != NULL) {
        delete this->right;
    }
}

Node* Node::buildTdag(std::vector<int> leafVals) {
    if (leafVals.size() == 0) {
        std::cerr << "Error: `leafVals` passed to `node.buildTdag()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }

    // linked list used to hold nodes while building (linked list chosen for efficiency)
    std::list<Node*> l;
    // initially load just the leaves into list
    for (int leafVal : leafVals) {
        l.push_back(new Node(leafVal, leafVal));
    }

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
                // `node1` becomes the left child of new parent node
                Node* parent = new Node(node1, node2);
                l.push_back(parent);
                l.erase(it);
                break;
            }
            if (node2->endVal + 1 == node1->startVal) {
                // `node2` becomes the left child of new parent node
                Node* parent = new Node(node2, node1);
                l.push_back(parent);
                l.erase(it);
                break;
            }
        }
    }

    return l.front();
}

std::ostream& operator << (std::ostream& os, const Node& node) {
    // todo
    return os;
}
