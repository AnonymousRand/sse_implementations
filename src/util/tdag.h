#pragma once

#include <list>
#include <set>
#include <unordered_set>

#include "util.h"

template <class T>
class TdagNode {
    private:
        Range<T> range;
        TdagNode<T>* left;
        TdagNode<T>* right;
        TdagNode<T>* extraParent;

        std::list<TdagNode<T>*> traverseHelper(std::unordered_set<TdagNode<T>*>& extraParents);
        TdagNode<T>* findSrcHelper(const Range<T>& targetRange);

    public:
        /**
         * Construct a `TdagNode` with the given `Range`, leaving its children `nullptr`.
         */
        TdagNode(const Range<T>& range);

        /**
         * Construct a `TdagNode` with the given children, setting its own `range`
         * to the union of its children's ranges.
         */
        TdagNode(TdagNode<T>* left, TdagNode<T>* right);
        
        ~TdagNode();

        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::list<TdagNode<T>*> traverse();

        /**
         * Find the single range cover of the leaves containing `range`.
         * If `range` not found in `this`, return `nullptr`.
         */
        TdagNode<T>* findSrc(Range<T> targetRange);

        /**
         * Get all leaf values from the subtree of `this`.
         */
        std::list<Range<T>> traverseLeaves();

        /**
         * Get all ancestors (i.e. covering nodes) of the leaf node with `leafRange` within the tree `this`,
         * including the leaf itself.
         */
        std::list<TdagNode<T>*> getLeafAncestors(const Range<T>& leafRange);

        /**
         * Get `this->range`.
         */
        Range<T> getRange() const;

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given maximum leaf value,
         * with consecutive size 1 ranges as leaves.
         */
        static TdagNode<T>* buildTdag(T& maxLeafVal);

        /**
         * Construct a TDAG (full binary tree + intermediate nodes) bottom-up from the given leaf values.
         * Leaf values must be disjoint but contiguous `Range`s; they are sorted in
         * ascending order by `set` based on the definition of the `<` operator for `Range`.
         */
        static TdagNode<T>* buildTdag(std::set<Range<T>>& leafVals);

        template <class T2>
        friend std::ostream& operator <<(std::ostream& os, TdagNode<T2>* node);
};
