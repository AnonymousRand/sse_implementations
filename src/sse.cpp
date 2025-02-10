#include "sse.h"

// have to explictly instantiate (give possible types to) template classes if separating .h and .cpp
template class SseClient<ustring, EncInd>;
template class SseServer<EncInd>;

template class SseClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>>;
template class SseServer<std::pair<EncInd, EncInd>>;

////////////////////////////////////////////////////////////////////////////////
// SseClient
////////////////////////////////////////////////////////////////////////////////

template <typename KeyType, typename EncIndType>
SseClient<KeyType, EncIndType>::SseClient(Db db) {
    this->db = db;
    if (this->db.empty()) {
        std::cerr << "Error: `db` passed to `SseClient.SseClient()` is empty :/" << std::endl;
        exit(EXIT_FAILURE);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SseServer
////////////////////////////////////////////////////////////////////////////////

template <typename EncIndType>
void SseServer<EncIndType>::setEncInd(EncIndType encInd) {
    this->encInd = encInd;
}

////////////////////////////////////////////////////////////////////////////////
// Controller
////////////////////////////////////////////////////////////////////////////////

template <typename EncIndType>
template <typename ClientType>
std::vector<Id> SseServer<EncIndType>::query(ClientType& client, KwRange query) {
    QueryToken queryToken = client.trpdr(query);
    return this->search(queryToken);
}
