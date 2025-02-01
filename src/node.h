#pragma once

#include <vector>

template <typename T>
class Node {
    private:
        Node* left;
        Node* right;
        T val;

    public:
        Node(T val);
        Node(std::vector<T> leaves);
        ~Node();
        T findSrc(std::tuple<T, T> range);
        std::vector<T> getLeavesOfSubtree(Node& node);
};
