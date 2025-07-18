#pragma once


#include <list>

#include "utils.h"


template <class T>
class TdagNode {
    public:
        /**
         * Construct a `TdagNode` (full binary tree + intermediate nodes) bottom-up up to and including the given max
         * leaf value, with consecutive size 1 ranges as leaves.
         */
        TdagNode(const Range<T>& leafValRange);
        ~TdagNode();

        /**
         * Find the single range cover of the leaves containing `range`.
         * If `range` not found in `this`, return `nullptr`.
         */
        Range<T> findSrc(Range<T> targetRange);

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with range `target` within the tree `this`,
         * including the leaf itself.
         */
        std::list<Range<T>> getLeafAncestors(const Range<T>& target);

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, TdagNode<T2>* node);

    private:
        Range<T> range;
        TdagNode<T>* left;
        TdagNode<T>* right;
        TdagNode<T>* extraParent;
        bool isExtraParent = false;

        /**
         * Construct a `TdagNode` with the given children, setting its own `range`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode<T>* left, TdagNode<T>* right);

        /**
         * Traverse subtree of `this` and return all traversed nodes in preorder.
         */
        std::list<TdagNode<T>*> traverse();
        std::list<TdagNode<T>*> traverseHelper(std::unordered_set<TdagNode<T>*>& extraParents);
        Range<T> findSrcHelper(const Range<T>& targetRange);
};
