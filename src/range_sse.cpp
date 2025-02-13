#include "pi_bas.h"
#include "range_sse.h"

template class IRangeSseClient<PiBasClient>;
template class IRangeSseServer<PiBasServer>;

template <typename Underlying>
IRangeSseClient<Underlying>::IRangeSseClient(Underlying underlying) : underlying(underlying) {}

template <typename Underlying>
IRangeSseServer<Underlying>::IRangeSseServer(Underlying underlying) : underlying(underlying) {}
