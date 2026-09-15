#include "utils/types/enc_ind/enc_ind_loc.h"

#include <cstdio>
#include <cstring>

#include "utils/benchmark.h"
#include "utils/types/basic_types.h"
#include "utils/types/enc_ind/enc_ind_base.h"


//------------------------------------------------------------------------------
// `EncIndBase`


void EncIndLoc::init(SseOper setupOper, bigint bcktCount, bigint bcktSize) {
    EncIndBase::init(setupOper, bcktCount * bcktSize);

    this->bcktCount = bcktCount;
    this->bcktSize = bcktSize;
}


void EncIndLoc::clear() {
    EncIndBase::clear();

    this->bcktCount = 0;
    this->bcktSize = 0;
}
