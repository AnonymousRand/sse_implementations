#pragma once

#include <list>
#include <set>
#include <unordered_set>

#include "util.h"

template <typename T>
class TdagNode {
    private:
        Range<T> range;
        TdagNode<T>* left;
        TdagNode<T>* right;
        TdagNode<T>* extraParent;

        /**
         * Traverse subtree of `this` and return all traversed nodes.
         */
        std::list<TdagNode<T>*> traverse();
        std::list<TdagNode<T>*> traverse(std::unordered_set<TdagNode<T>*>& extraParents);

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
         * Find the single range cover of the leaves containing `range`.
         * If `range` not found in `this`, return `nullptr`.
         */
        TdagNode<T>* findSrc(const Range<T>& targetRange);

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

        template <typename T2>
        friend std::ostream& operator <<(std::ostream& os, TdagNode<T2>* node);
};
