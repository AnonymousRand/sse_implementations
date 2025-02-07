#pragma once

#include <forward_list>
#include <iostream>
#include <tuple>
#include <set>
#include <vector>

#include "util.h"

class TdagNode {
    private:
        Kw startVal;
        Kw endVal;
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
        TdagNode(Kw startVal, Kw endVal);

        /**
         * Construct a `TdagNode` with the given children, setting its own `startVal` and `endVal`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode* left, TdagNode* right);
        
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode* findSrc(KwRange range);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::vector<KwRange> traverseSrc();

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but continguous `KwRange`s;
         * sorted in ascending order by `std::set` based on `<` operator for `KwRange`
         */
        static TdagNode* buildTdag(std::set<KwRange> leafVals);

        // overload for debugging
        friend std::ostream& operator << (std::ostream& os, const TdagNode* node);
};
