#pragma once

#include <concepts>

#include "utils/types/db/db.h"
#include "utils/types/tdag.h"
#include "utils/types/tuple.h"


namespace log_src::utils {


template <IsDbTuple DbTuple>
void buildTdagDbFromLeaves(
    Db<DbTuple>& db, TdagNode<typename DbTuple::DbKwType>*& tdag, bool shouldPadDb = false
);


template <IsDbTuple DbTuple>
void replTdagDb(Db<DbTuple>& db, const TdagNode<typename DbTuple::DbKwType>* tdag);


} // namespace `log_src::utils`
