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
    //--------------------------------------------------------------------------
    // constructors/destructors

    // default constructor needed for `init()`
    TdagNode() = default;

    ~TdagNode();

    //--------------------------------------------------------------------------
    // rule of five

    // delete all these to prevent copying and moving! as they would then require annoying
    // management of all the recursive pointer members
    // this means TDAGs can only be instantiated as pointers!
    
    // copy constructor
    TdagNode(const TdagNode& other) = delete;

    // copy assignment operator
    TdagNode& operator =(const TdagNode& other) = delete;

    // move constructor
    TdagNode(TdagNode&& other) noexcept = delete;

    // move assignment operator
    TdagNode& operator =(TdagNode&& other) noexcept = delete;

    //--------------------------------------------------------------------------
    // interface

    /**
     * construct a `TdagNode` (full binary tree + intermediate nodes) bottom-up with the
     * given range as the range of leaf nodes.
     */
    static TdagNode* create(const Range<T>& leafRange);

    /**
     * find the single range cover of the leaves containing `range`.
     * if `range` is not found in `this`, return `nullptr`.
     */
    // (`targetRange` is supposed to be pass by value since it may be locally modified)
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
    TdagNode* left = nullptr;
    TdagNode* right = nullptr;
    TdagNode* extraParent = nullptr;
    bool isExtraParent = false;

    /**
     * construct a `TdagNode` with the given children, setting its own `range`
     * to the union of its children's ranges.
     */
    TdagNode(TdagNode* left, TdagNode* right);

    /**
     * traverse subtree of `this` and return all traversed nodes in preorder.
     */
    std::list<const TdagNode*> traverse() const;
    std::list<const TdagNode*> traverseHelper(std::unordered_set<TdagNode*>& extraParents) const;

    Range<T> findSrcHelper(const Range<T>& targetRange) const;
};


//==============================================================================
// utils
//==============================================================================


namespace utils::tdag {


bigint calcTdagTupleCount(bigint leafCount);


} // namespace `utils::tdag`
