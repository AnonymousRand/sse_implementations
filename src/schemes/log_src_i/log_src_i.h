#pragma once

#include <concepts>

#include "schemes/interfaces/sse.h"
#include "schemes/log_src_i/log_src_i_base.h"

#include "utils/db.h"
#include "utils/range.h"
#include "utils/sse_utils.h"
#include "utils/tdag.h"


template <template <class ...> class Underly> requires IsSse<Underly<Record<>, Kw>>
class LogSrcI : public LogSrcIBase<Underly> {
public:
    using LogSrcIBase<Underly>::LogSrcIBase;

    //----------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - records in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - records in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Record<>, Kw>& db) override;
};
