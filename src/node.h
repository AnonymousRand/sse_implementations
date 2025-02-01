#pragma once

#include <vector>

class Node {
    private:
        Node* left;
        Node* right;
        int val;

    public:
        Node(int val);
        ~Node();

        /**
         * Create a TDAG (full binary tree + intermediate nodes) from the given leaf values.
         * Leaf values must be contiguous integers sorted in ascending order.
         */
        Node* buildTdag(std::vector<int> leafVals);

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        int findSrc(std::tuple<int, int> range);

        /**
         * Get all leaf values from the subtree of `node`.
         */
        std::vector<int> getSubtreeLeafVals(Node* node);
};
