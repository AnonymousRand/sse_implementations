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

std::pair<EncInd, EncInd> buildIndex(std::pair<ustring, ustring> key, Db db) {
    EncInd encInd1;
    EncInd encInd2;
    
    // build TDAG1 over keywords
    Kw maxKw = -1;
    for (Doc doc : db) {
        KwRange kwRange = std::get<1>(doc);
        if (kwRange.end > maxKw) {
            maxKw = kwRange.end;
        }
    }
    this->tdag1 = TdagNode::buildTdag(maxKw);

    // replicate every document to all nodes/keywords ranges in TDAG1 that cover it
    // `db1` contains individual entries in each `values` or `documents` column of index 1: pairs (kw, id range)
    std::vector<std::pair<Kw, Ind1Val>> db1;
    for (Doc doc : db) {
        Id id = std::get<0>(doc);
        KwRange kwRange = std::get<1>(doc);
        std::list<TdagNode*> ancestors = this->tdag->getLeafAncestors(kwRange);
        for (TdagNode* ancestor : ancestors) {
            KwRange kwRange = ancestor->getKwRange();
            if (index.count(id) == 0) {
                index[id] = std::vector<KwRange> {kwRange};
            } else {
                index[id].push_back(kwRange);
            }
        }
    }

    return this->underlying.buildIndex(key, processedDb);
}

QueryToken trpdr1(ustring key1, KwRange kwRange) {

}

QueryToken trpdr(ustring key2, std::vector<Ind1Val> choices) {

}

////////////////////////////////////////////////////////////////////////////////
// LogSrciServer
////////////////////////////////////////////////////////////////////////////////
