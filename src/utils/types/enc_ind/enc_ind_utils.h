#pragma once

#include <utility>

#include "utils/types/ustring.h"


/**
 * encrypted indexes are a collection of `std::pair<ustring, std::pair<ustring, ustring>>`
 * (aka `EncIndEntry`) pairs, corresponding to `std::pair<key, std::pair<encrypted data, IV>>`.
 */
using EncIndVal   = std::pair<ustring, ustring>;
using EncIndEntry = std::pair<ustring, EncIndVal>;


namespace utils::enc_ind {


ustring toUstr(const EncIndEntry& encIndEntry);


} // namespace `utils::enc_ind`
