#pragma once

#include <concepts>

#include "schemes/interfaces/sse.h"
#include "schemes/log_src_i/log_src_i_base.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/tdag.h"
#include "utils/types.h"


template <template <class ...> class Underly> requires IsSse<Underly<Tuple<>, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
public:
    using LogSrcIBase<Underly>::LogSrcIBase;

    //----------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - tuples in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - tuples in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Tuple<>>& db) override;
};
