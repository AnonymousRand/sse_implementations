#pragma once

#include "schemes/log_src_i/log_src_i_base.h" 
#include "schemes/log_src_i_star/log_src_i_star_underly.h"

#include "types/basic_types.h"
#include "types/db/db.h"
#include "types/tuple.h"


class LogSrcIStar : public LogSrcIBase<log_src_i_star::Underly> {
public:
    //--------------------------------------------------------------------------
    // `ISse`

    void setup(int secParam, const Db<Tuple<>>& db) override;
};
