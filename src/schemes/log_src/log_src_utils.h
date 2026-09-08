#pragma once

#include <concepts>

#include "types/db/db.h"
#include "types/tdag.h"
#include "types/tuple.h"


namespace log_src::utils {


template <IsDbTuple DbTuple>
void buildTdagDbFromLeaves(
    Db<DbTuple>& db, TdagNode<typename DbTuple::DbKwType>*& tdag, bool shouldPadDb = false
);


template <IsDbTuple DbTuple>
void replTdagDb(Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag);


} // namespace `log_src::utils`
