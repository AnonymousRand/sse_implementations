#include "node.h"

#include <queue>

Node::Node(int val) {
    this->val = val;
    this->left = NULL;
    this->right = NULL;
}

Node::~Node() {
    if (this->left != NULL) {
        delete this->left;
    }
    if (this->right != NULL) {
        delete this->right;
    }
}

Node* buildTdag(std::vector<int> leafVals) {
    // queue used to hold nodes while building
    std::queue<Node*> q;
    // initially load just the leaves into queue
    for (int leafVal : leafVals) {
        q.push(new Node(leafVal));
    }

    while (!q.empty()) {
        
    }
}
