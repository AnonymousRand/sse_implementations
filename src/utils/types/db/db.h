#pragma once

#include <concepts>
#include <type_traits>

#include "config.h"

#include "utils/types/db/db_disk.h"
#include "utils/types/db/db_ram.h"
#include "utils/types/range.h"
#include "utils/types/tuple.h"


// make sure that `Db` is always a type that inherits from `IDb`!
template <IsDbTuple DbTuple = Tuple<>>
using Db = std::conditional<
    config::SHOULD_STORE_DBS_ON_DISK, DbDisk<DbTuple>, DbRam<DbTuple>
>::type;
