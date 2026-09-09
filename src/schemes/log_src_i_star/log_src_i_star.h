#pragma once

#include "schemes/log_src_i/log_src_i_base.h" 
#include "schemes/log_src_i_star/log_src_i_star_underly.h"

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/tuple.h"


class LogSrcIStar : public LogSrcIBase<log_src_i_star::Underly> {
public:
    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Tuple<>>& db) override;
};
