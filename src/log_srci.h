#pragma once

#include "pi_bas.h"

typedef std::tuple<Kw, IdRange> Ind1Val;
typedef std::unordered_map<KwRange, std::vector<Ind1Val>> Ind1;
// todo do these datatypes need to be revised based on new intuitoin from 2/5 lecture about tdag2 and stuff?
typedef std::unordered_map<KwRange, std::vector<Id>> Ind2;

class LogSrciClient : public PiBasClient<std::pair<ustring, ustring>, std::pair<EncInd, EncInd>> {
    public:
        LogSrciClient(Db db);

        void setup(int secParam) override;
        BaseEncIndType buildIndex() override;
        // interactivity messes up the API >:(
        QueryToken trpdr1(KwRange kwRange);
        QueryToken trpdr2(std::vector<Ind1Val> choices);
};

class LogSrciServer : public PiBasServer<std::pair<EncInd, EncInd>> {
    public:
        std::vector<Ind1Val> search1(QueryToken queryToken);
        std::vector<Id> search2(QueryToken queryToken);
};

// overload since search is now interactive
std::vector<Id> query(LogSrciClient& client, LogSrciServer& server, KwRange query);
