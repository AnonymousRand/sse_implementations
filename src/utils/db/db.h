#pragma once

#include <concepts>
#include <type_traits>
#include <unordered_map>

#include "config.h"

#include "utils/db/db_disk.h"
#include "utils/db/db_ram.h"
#include "utils/range.h"
#include "utils/tuple.h"


// make sure that `Db` is always a type that inherits from `IDb`!
template <IsDbTuple DbTuple = Tuple<>>
using Db = std::conditional<
    config::SHOULD_STORE_DBS_ON_DISK, DbDisk<DbTuple>, DbRam<DbTuple>
>::type;


template <IsDbTuple DbTuple = Tuple<>>
using Ind = std::unordered_map<Range<typename DbTuple::DbKwType>, Db<DbTuple>>;
