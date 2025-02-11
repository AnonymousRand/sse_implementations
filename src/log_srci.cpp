#include "log_srci.h"

////////////////////////////////////////////////////////////////////////////////
// LogSrciClient
////////////////////////////////////////////////////////////////////////////////

LogSrciClient::LogSrciClient(ISseClient<ustring, EncInd>& underlying)
        : IRangeSseClient<ustring, EncInd>(underlying) {};

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

//std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db<Id, KwRange> db) {
//    EncInd encInd1;
//    EncInd encInd2;
//    
//    // build TDAG1 over keywords
//    Kw maxKw = -1;
//    for (auto entry : db) {
//        KwRange kwRange = std::get<1>(entry);
//        if (kwRange.second > maxKw) {
//            maxKw = kwRange.second;
//        }
//    }
//    this->tdag1 = TdagNode<Kw>::buildTdag(maxKw);
//
//    // replicate every document to all nodes/keywords ranges in TDAG1 that cover it
//    // `db1` contains individual entries in each `values` or `documents` column of index 1: pairs (kw, id range)
//    // todo sort out and finish
//    std::vector<std::pair<KwRange, std::pair<KwRange, IdRange>>> db1;
//    for (auto entry : db) {
//    }
//
//    return this->underlying.buildIndexGeneric(key, processedDb);
//}

QueryToken trpdr1(ustring key1, KwRange kwRange) {

}

QueryToken trpdr(ustring key2, std::vector<Ind1Val> choices) {

}

////////////////////////////////////////////////////////////////////////////////
// LogSrciServer
////////////////////////////////////////////////////////////////////////////////
