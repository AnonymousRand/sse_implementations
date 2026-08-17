#pragma once

#include <concepts>
#include <iostream>
#include <list>
#include <unordered_set>

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


//==============================================================================
// `TdagNode`
//==============================================================================


template <std::integral T>
class TdagNode {
public:
    /**
     * construct a `TdagNode` (full binary tree + intermediate nodes) bottom-up up to and
     * including the given max leaf value, with consecutive size 1 ranges as leaves.
     */
    TdagNode(const Range<T>& leafRange);
    TdagNode(T leafRangeStart, T leafRangeEnd);

    ~TdagNode();

    /**
     * find the single range cover of the leaves containing `range`.
     * if `range` not found in `this`, return `nullptr`.
     */
    Range<T> findSrc(Range<T> targetRange) const;

    /**
     * get all ancestors (i.e. covering nodes) of the leaf node with range `target`
     * within the tree `this`, including the leaf itself.
     */
    std::list<Range<T>> getLeafAncestors(const Range<T>& target) const;

    template <std::integral T2>
    friend std::ostream& operator <<(std::ostream& os, TdagNode<T2>* node);

private:
    Range<T> range;
    TdagNode<T>* left = nullptr;
    TdagNode<T>* right = nullptr;
    TdagNode<T>* extraParent = nullptr;
    bool isExtraParent = false;

    /**
     * construct a `TdagNode` with the given children, setting its own `range`
     * to the union of its children's ranges.
     */
    TdagNode(TdagNode<T>* left, TdagNode<T>* right);

    /**
     * traverse subtree of `this` and return all traversed nodes in preorder.
     */
    std::list<const TdagNode<T>*> traverse() const;
    std::list<const TdagNode<T>*> traverseHelper(
        std::unordered_set<TdagNode<T>*>& extraParents
    ) const;

    Range<T> findSrcHelper(const Range<T>& targetRange) const;
};


//==============================================================================
// utils
//==============================================================================


namespace utils::tdag {


bigint calcTdagTupleCount(bigint leafCount);


template <IsDbTuple DbTuple>
void buildTdagAndDb(
    TdagNode<typename DbTuple::DbKwType>*& tdag, Db<DbTuple>& db, bool shouldPadDb = false
);


template <IsDbTuple DbTuple>
void replDbForTdag(Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag);


} // namespace `utils::tdag`
