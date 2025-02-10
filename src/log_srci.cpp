#include "log_srci.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrciClient
////////////////////////////////////////////////////////////////////////////////

void LogSrciClient::setup(int secParam) {
    unsigned char* key = new unsigned char[secParam];

    int res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key1 = toUstr(key, secParam);
    res = RAND_priv_bytes(key, secParam);
    if (res != 1) {
        handleOpenSslErrors();
    }
    this->key2 = toUstr(key, secParam);

    delete[] key;
}

void 

////////////////////////////////////////////////////////////////////////////////
// LogSrciServer
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Controller
////////////////////////////////////////////////////////////////////////////////

std::vector<Id> query(LogSrciClient& client, LogSrciServer& server, KwRange query) {
    QueryToken queryToken1 = client.trpdr1(query);
    std::vector<Ind1Val> results1 = server.search1(queryToken);
    QueryToken queryToken2 = client.trpdr2(results1);
    return server.search2(queryToken);
}
