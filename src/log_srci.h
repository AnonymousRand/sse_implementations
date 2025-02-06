#pragma once

#include "log_src.h"


typedef std::tuple<int, IdRange> Index1Val;
typedef std::tuple<KwRange, std::vector<Index1Val>> Index1;
// todo do these datatypes need to be revised based on new intuitoin from 2/5 lecture about tdag2 and stuff?
typedef std::tuple<IdRange, std::vector<int>> Index2;


class LogSrciClient : public LogSrcClient {
    private:
        unsigned char* key1;
        unsigned char* key2;

    public:
        void setup(int secParam);
        std::tuple<EncIndex, EncIndex> buildIndex();
        QueryToken trpdr1(KwRange kwRange);
        QueryToken trpdr2(IdRange idRange);
};


class LogSrciServer : public LogSrcServer {
    private:
        Index1 index1;
        Index2 index2;

    public:
        std::vector<Index1Val> search1(QueryToken queryToken);
        std::vector<int> search2(QueryToken queryToken);
};
