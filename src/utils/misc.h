#pragma once

#include <concepts>
#include <vector>

#include "utils/types/basic_types.h"
#include "utils/types/doc.h"


namespace utils::misc {


template <IsDbDoc DbDoc>
void cleanUpResults(std::vector<DbDoc>& results);


bigint roundUpToPowOf2(bigint n);


} // namespace `utils::misc`
