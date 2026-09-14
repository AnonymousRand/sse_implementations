#include "utils/types/enc_ind/enc_ind_loc.h"

#include <cstdio>
#include <cstring>

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"


//------------------------------------------------------------------------------
// `EncIndBase`


void EncIndLoc::init(SseOper setupOper, bigint bcktSize, bigint bcktCount) {
    EncIndBase::init(setupOper, bcktSize * bcktCount);

    this->bcktSize = bcktSize;
    this->bcktCount = bcktCount;
}


void EncIndLoc::clear() {
    EncIndBase::clear();

    this->bcktSize = 0;
    this->bcktCount = 0;
}
