#include "log_srci.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrciClient
////////////////////////////////////////////////////////////////////////////////

LogSrciClient::LogSrciClient(SseClient<ustring, EncInd>& underlying)
        : RangeSseClient<ustring, EncInd>(underlying) {};

std::pair<ustring, ustring> setup(int secParam) {
    unsigned char* key1 = new unsigned char[secParam];
    unsigned char* key2 = new unsigned char[secParam];
    int res1 = RAND_priv_bytes(key1, secParam);
    int res2 = RAND_priv_bytes(key2, secParam);
    if (res1 != 1 || res2 != 1) {
        handleOpenSslErrors();
    }

    ustring ustrKey1 = toUstr(key1, secParam);
    ustring ustrKey2 = toUstr(key2, secParam);
    delete[] key1, key2;
    return std::pair<ustring, ustring> {ustrKey1, ustrKey2};
}

std::pair<EncIndex, EncIndex> buildIndex(std::pair<ustring, ustring> key, Db db) {
    
}

QueryToken trpdr1(ustring key1, KwRange kwRange) {

}

QueryToken trpdr2(ustring key2, std::vector<Ind1Val> choices) {

}

////////////////////////////////////////////////////////////////////////////////
// LogSrciServer
////////////////////////////////////////////////////////////////////////////////
