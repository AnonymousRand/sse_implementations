#pragma once

#include <list>
#include <set>

#include "util.h"

template <class T>
class TdagNode {
    private:
        Range<T> range;
        TdagNode<T>* left;
        TdagNode<T>* right;
        TdagNode<T>* extraParent;

        /**
         * Construct a `TdagNode` with the given `Range`, leaving its children `nullptr`.
         */
        TdagNode(const Range<T>& range);

        /**
         * Construct a `TdagNode` with the given children, setting its own `range`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode<T>* left, TdagNode<T>* right);

        std::list<TdagNode<T>*> traverseHelper(std::unordered_set<TdagNode<T>*>& extraParents);
        Range<T> findSrcHelper(const Range<T>& targetRange);

    public:
        /**
         * Construct a `TdagNode` (full binary tree + intermediate nodes) bottom-up from the given leaf count,
         * with consecutive size 1 ranges as leaves.
         */
        TdagNode(T leafCount);
        
        ~TdagNode();

        /**
         * Traverse subtree of `this` and return all traversed nodes in preorder.
         */
        std::list<TdagNode<T>*> traverse();

        /**
         * Find the single range cover of the leaves containing `range`.
         * If `range` not found in `this`, return `nullptr`.
         */
        Range<T> findSrc(Range<T> targetRange);

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with range `leafRange` within the tree `this`,
         * including the leaf itself.
         */
        std::list<TdagNode<T>*> getLeafAncestors(const Range<T>& leafRange);

        /**
         * Get `this->range`.
         */
        Range<T> getRange() const;

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, TdagNode<T2>* node);
};
