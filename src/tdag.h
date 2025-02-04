#pragma once

#include <forward_list>
#include <iostream>
#include <tuple>
#include <vector>


class TdagNode {
    private:
        int startVal;
        int endVal;
        TdagNode* left;
        TdagNode* right;
        TdagNode* extraParent;


        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::forward_list<TdagNode*> traverse();


    public:
        /**
         * Construct a `TdagNode` with the given `startVal` and `endVal`, leaving its children `nullptr`.
         */
        TdagNode(int startVal, int endVal);


        /**
         * Construct a `TdagNode` with the given children, setting its own `startVal` and `endVal`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode* left, TdagNode* right);
        

        ~TdagNode();


        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode* findSrc(std::tuple<int, int> range);


        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::vector<int> traverseSrc();


        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be contiguous integers sorted in ascending order.
         */
        static TdagNode* buildTdag(std::vector<int> leafVals);

        friend std::ostream& operator << (std::ostream& os, const TdagNode* node);
};
