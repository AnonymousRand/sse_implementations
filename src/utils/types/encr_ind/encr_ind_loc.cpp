#include "utils/types/encr_ind/encr_ind_loc.h"

#include <cstdio>
#include <cstring>

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/encr_ind/encr_ind_base.h"


//------------------------------------------------------------------------------
// `EncrIndBase`


void EncrIndLoc::init(SseOper setupOper, bigint bcktCount, bigint bcktSize) {
    EncrIndBase::init(setupOper, bcktCount * bcktSize);

    this->bcktCount = bcktCount;
    this->bcktSize = bcktSize;
}


void EncrIndLoc::clear() {
    EncrIndBase::clear();

    this->bcktCount = 0;
    this->bcktSize = 0;
}
