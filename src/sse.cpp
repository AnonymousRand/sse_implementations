#include "sse.h"

////////////////////////////////////////////////////////////////////////////////
// IRangeSse
////////////////////////////////////////////////////////////////////////////////

// have to explicitly instantiate templates (declare all possible types it could take)
// to make template class implementation work from a separate file
// todo can we delete these?
template class IRangeSseClient<ustring, EncInd>;
template class IRangeSseServer<EncInd>;

template class IRangeSseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>;
template class IRangeSseServer<std::pair<EncInd, EncInd>>;

template <typename KeyType, typename EncIndType>
IRangeSseClient<KeyType, EncIndType>::IRangeSseClient(ISseClient<KeyType, EncIndType>& underlying)
        : underlying(underlying) {};

template <typename EncIndType>
IRangeSseServer<EncIndType>::IRangeSseServer(ISseServer<EncIndType>& underlying)
        : underlying(underlying) {};
