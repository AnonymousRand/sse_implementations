#include "log_srci.h"

LogSrciClient::LogSrciClient(Db db) : PiBasClient(db) {};

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
