#include <iostream>
#include <string.h>

#include <openssl/rand.h>

#include "sse.h"

void SseServer::setEncIndex(EncIndex encIndex) {
    this->encIndex = encIndex;
}

SseClient::SseClient(Db db) {
    this->db = db;
    for (auto pair : db) {
        this->uniqueKwRanges.insert(pair.second);
    }
}
