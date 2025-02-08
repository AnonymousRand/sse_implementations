#pragma once

#include <iostream>
#include <list>
#include <set>

#include "util.h"

class TdagNode {
    private:
        KwRange kwRange;
        TdagNode* left;
        TdagNode* right;
        TdagNode* extraParent;

        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::list<TdagNode*> traverse();

    public:
        /**
         * Construct a `TdagNode` with the given `KwRange`, leaving its children `nullptr`.
         */
        TdagNode(KwRange kwRange);

        /**
         * Construct a `TdagNode` with the given children, setting its own `kwRange`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode* left, TdagNode* right);
        
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         */
        TdagNode* findSrc(KwRange kwRange);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::list<KwRange> traverseLeaves();

        /**
         * Get all covering nodes (i.e. ancestors) of the leaf node with `leafKwRange` within the tree given by `this`.
         */
        std::list<TdagNode*> getAllCoversForLeaf(KwRange leafKwRange);

        /**
         * Getter for `this->kwRange`.
         */
        KwRange getKwRange();

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
