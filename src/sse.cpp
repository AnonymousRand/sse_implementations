// temp compilation instructions: g++ pi_bas.cpp -lcrypto -o a
#include "sse.h"

#include <iostream>
#include <openssl/rand.h>
#include <string.h>

void SseServer::setEncIndex(EncIndex encIndex) {
    this->encIndex = encIndex;
}

SseClient::SseClient(Db db) {
    this->db = db;
    for (auto pair : db) {
        this->uniqueKws.insert(pair.second);
    }
}
