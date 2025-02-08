#pragma once

#include <iostream>
#include <list>
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
        std::list<TdagNode*> traverse();

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
         * Convert the root node of `this` into a `KwRange`.
         */
        KwRange getRootKwRange();

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given maximum leaf value,
         * with consecutive, size 1 ranges as leaves.
         */
        static TdagNode* buildTdag(Kw maxLeafVal);

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but contiguous `KwRange`s; they are sorted in
         * ascending order by `std::set` based on the definition of the `<` operator for `KwRange`.
         */
        static TdagNode* buildTdag(std::set<KwRange> leafVals);

        // overload for debugging
        friend std::ostream& operator << (std::ostream& os, TdagNode* node);
};
