#pragma once

#include <iostream>
#include <vector>

class Node {
    private:
        int startVal;
        int endVal;
        Node* left;
        Node* right;

    public:
        /**
         * Construct a `Node` with the given `startVal` and `endVal`, leaving its children `nullptr`.
         */
        Node(int startVal, int endVal);

        /**
         * Construct a `Node` with the given children, setting its own `startVal` and `endVal`
         * to the union of its children's ranges.
         */
        Node(Node* left, Node* right);
        
        ~Node();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        //int findSrc(std::tuple<int, int> range);

        /**
         * Get all leaf values from the subtree of `node`.
         */
        //std::vector<int> getSubtreeLeafVals(Node* node);

        /**
         * Create a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be contiguous integers sorted in ascending order.
         */
        static Node* buildTdag(std::vector<int> leafVals);

        friend std::ostream& operator << (std::ostream& os, const Node& node);
};
