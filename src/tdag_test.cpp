#include "tdag.h"

int main() {
    std::vector<int> leafVals = {0, 1 ,2 ,3, 4, 5, 6, 7};
    TdagNode* tdag = TdagNode::buildTdag(leafVals);
    std::cout << tdag->findSrc(std::tuple<int, int> {2, 4}) << std::endl;
}
