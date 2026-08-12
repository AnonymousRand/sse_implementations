#pragma once

#include "schemes/log_src_i/log_src_i_base.h" 
#include "schemes/log_src_i_star/underly.h"

#include "utils/db.h"
#include "utils/sse_utils.h"


class LogSrcIStar : public LogSrcIBase<log_src_i_star::Underly> {
public:
    using LogSrcIBase<log_src_i_star::Underly>::LogSrcIBase;

    //----------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - records in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - records in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Record<>, Kw>& db) override;
};
