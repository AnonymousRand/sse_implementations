#pragma once

#include "pi_bas.h"

typedef std::tuple<Kw, IdRange> Index1Val;
typedef std::unordered_map<KwRange, std::vector<Index1Val>> Index1;
// todo do these datatypes need to be revised based on new intuitoin from 2/5 lecture about tdag2 and stuff?
typedef std::unordered_map<kwRange, std::vector<Id>> Index2;

class LogSrciClient : public PiBasClient {
    protected:
        ustring key1;
        ustring key2;

    public:
        LogSrciClient(Db db);

        void setup(int secParam) override;
        std::pair<EncIndex, EncIndex> buildIndex();
        QueryToken trpdr1(KwRange kwRange);
        QueryToken trpdr2(std::vector<Index1Val> choices);
};

class LogSrciServer : public PiBasServer {
    protected:
        EncIndex encIndex1;
        EncIndex encIndex2;

    public:
        void setEncIndex(std::pair<EncIndex, EncIndex> encIndexes);

        std::vector<Index1Val> search1(QueryToken queryToken);
        std::vector<Id> search2(QueryToken queryToken);
};
