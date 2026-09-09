#pragma once

#include <concepts>

#include "schemes/interfaces/i_sse.h"
#include "schemes/log_src_i/log_src_i_base.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>>>
class LogSrcI : public LogSrcIBase<Underly> {
public:
    //--------------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - tuples in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - tuples in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Tuple<>>& db) override;
};
