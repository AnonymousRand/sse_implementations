#include "tdag.h"


int main() {
    std::vector<int> leafVals;
    for (int i = 0; i < 500000; i++) {
        leafVals.push_back(i);
    }
    TdagNode* tdag = TdagNode::buildTdag(leafVals);
    TdagNode* src = tdag->findSrc(std::tuple<int, int> {22323, 22344});
    std::cout << src << std::endl;
    std::vector<int> leafValsOfSrc = src->traverseSrc();
}
