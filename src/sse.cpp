#include "sse.h"

// have to explicitly instantiate templates (declare all possible types it could take)
// to make template class implementation work from a separate file (i have never wished more that i was using Java)
template class RangeSseClient<ustring, EncInd>;
template class RangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>;
template class RangeSseServer<EncInd>;
template class RangeSseServer<std::pair<EncInd, EncInd>>;

template <typename KeyType, typename EncIndType>
RangeSseClient<KeyType, EncIndType>::RangeSseClient(SseClient<KeyType, EncIndType>& underlying)
        : underlying(underlying) {};

template <typename EncIndType>
RangeSseServer<EncIndType>::RangeSseServer(SseServer<EncIndType>& underlying)
        : underlying(underlying) {};
