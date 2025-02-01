#include "node.h"

int main() {
    std::vector<int> leafVals = {0, 1 ,2 ,3, 4};
    Node* tdag = Node::buildTdag(leafVals);
}
