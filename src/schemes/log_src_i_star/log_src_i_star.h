#pragma once

#include "schemes/log_src_i/log_src_i_base.h" 
#include "schemes/log_src_i_star/log_src_i_star_underly.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


class LogSrcIStar : public LogSrcIBase<log_src_i_star::Underly> {
private:
    using DbKw = typename LogSrcIBase<log_src_i_star::Underly>::DbKw;

public:
    using LogSrcIBase<log_src_i_star::Underly>::LogSrcIBase;

    //--------------------------------------------------------------------------
    // `ISse`

    /**
     * preconditions:
     *     - tuples in `db` must have size 1 `Kw` ranges, i.e. a singular `Kw` value.
     *     - tuples in `db` cannot have keyword equal to `DUMMY`.
     */
    void setup(int secParam, const Db<Tuple<>>& db) override;
};
