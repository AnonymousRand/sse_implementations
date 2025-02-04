#pragma once

#include <forward_list>
#include <iostream>
#include <vector>

class Node {
    private:
        int startVal;
        int endVal;
        Node* left;
        Node* right;

        /**
         * Traverse subtree of `root` and return all nodes in that subtree.
         */
        std::forward_list<Node*> traverse(Node* root);

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
        // ideas: naive way is to traverse whole tree, return all nodes, check one by one
        // better: can probably do some modified binary search for left and right values separately
        // this might not work with tdag intermediate nodes though
        //int findSrc(std::tuple<int, int> range);

        /**
         * Get all leaf values from the subtree of `root`.
         */
        //std::vector<int> getSubtreeLeafVals(Node* root);

        /**
         * Construct a `full` binary tree bottom-up from the given leaf values.
         * Leaf values must be contiguous integers sorted in ascending order.
         */
        static Node* buildFromLeafVals(std::vector<int> leafVals);

        friend std::ostream& operator << (std::ostream& os, const Node& node);
};

class TdagNode : public Node {
    private:
        TdagNode* extraParent;

        std::forward_list<Node*> traverse(Node* root);

    public:
        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be contiguous integers sorted in ascending order.
         */
        static TdagNode* buildFromLeafVals(std::vector<int> leafVals);
};
