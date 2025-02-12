#include "sse.h"

// have to explicitly instantiate templates (declare all possible types it could take)
// to make template class implementation work from a separate file
// todo can we delete these?
template class ISseClient<ustring, EncInd>;
template class ISseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>;
