#pragma once

#include "schemes/log_src_i/log_src_i_base.h" 
#include "schemes/log_src_i_star/underly.h"

#include "utils/db.h"
#include "utils/tuple.h"
#include "utils/types.h"


class LogSrcIStar : public LogSrcIBase<log_src_i_star::Underly> {
private:
    using DbKw = typename LogSrcIBase<log_src_i_star::Underly>::DbKw;

public:
    using LogSrcIBase<log_src_i_star::Underly>::LogSrcIBase;

    //----------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - tuples in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - tuples in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Tuple<>>& db) override;
};
