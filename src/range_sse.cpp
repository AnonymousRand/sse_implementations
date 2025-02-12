#include "range_sse.h"

// todo can we delete these?
template class IRangeSseClient<ustring, EncInd, PiBasClient>;
template class IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>, PiBasClient>;
template class IRangeSseServer<PiBasServer>;

template <typename KeyType, typename EncIndType, typename Underlying>
IRangeSseClient<KeyType, EncIndType, Underlying>::IRangeSseClient(Underlying underlying) : underlying(underlying) {}

template <typename Underlying>
IRangeSseServer<Underlying>::IRangeSseServer(Underlying underlying) : underlying(underlying) {}
