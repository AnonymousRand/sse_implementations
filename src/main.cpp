// temp compilation `g++ main.cpp sse.cpp range_sse.cpp pi_bas.cpp log_src.cpp log_srci.cpp util/*.cpp -lcrypto -o a`
#include <cmath>
#include <random>
#include <type_traits>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"
#include "sse.h"

void exp1(ISseClient<ustring, EncInd>& client, ISseServer& server, Db<> db) {
    ustring key = client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    EncInd encInd = client.buildIndex(key, db);

    KwRange query {3, 5};
    std::cout << "Querying " << query << "..." << std::endl;
    QueryToken queryToken = client.trpdr(key, query);
    std::vector<Id> results = server.search(encInd, queryToken);

    std::cout << "Results are:";
    for (Id result : results) {
        std::cout << " " << result;
    }
    std::cout << std::endl;
}

template <typename UnderlyingClientType, typename UnderlyingServerType>
void exp1(LogSrciClient<UnderlyingClientType>& client, LogSrciServer<UnderlyingServerType>& server, Db<> db) {
    std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    std::pair<EncInd, EncInd> encInds = client.buildIndex(keys, db);

    KwRange query {3, 5};
    std::cout << "Querying " << query << "..." << std::endl;
    QueryToken queryToken1 = client.trpdr(keys.first, query);
    std::vector<SrciDb1Doc> results1 = server.search1(encInds.first, queryToken1);
    QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
    std::vector<Id> results2 = server.search(encInds.second, queryToken2);

    std::cout << "Results are:";
    for (Id result : results2) {
        std::cout << " " << result;
    }
    std::cout << std::endl;
}

int main() {
    // experiment 1: db of size 2^20 and vary range sizes
    //const int dbSize = pow(2, 3);
    //Db<> db;
    //std::random_device dev;
    //std::mt19937 rng(dev());
    //std::uniform_int_distribution<int> dist(0, dbSize - 1);
    //std::cout << "----- Dataset -----" << std::endl;
    //for (int i = 0; i < dbSize; i++) {
    //    //Kw kw = dist(rng);
    //    Kw kw = i;
    //    db.push_back(Doc {Id(i), KwRange {kw, kw}});
    //    //std::cout << i << ": " << db.find(i)->second << std::endl;
    //}

    Db<> db;
    db.push_back(std::pair<Id, KwRange> {Id(0), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(1), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(2), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(3), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(4), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(5), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(6), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(7), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(8), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(9), KwRange {2, 2}});
    db.push_back(std::pair<Id, KwRange> {Id(10), KwRange {4, 4}});
    db.push_back(std::pair<Id, KwRange> {Id(11), KwRange {5, 5}});
    db.push_back(std::pair<Id, KwRange> {Id(12), KwRange {5, 5}});
    db.push_back(std::pair<Id, KwRange> {Id(13), KwRange {6, 6}});
    db.push_back(std::pair<Id, KwRange> {Id(14), KwRange {6, 6}});
    db.push_back(std::pair<Id, KwRange> {Id(15), KwRange {7, 7}});

    PiBasClient piBasClient;
    PiBasServer piBasServer;
    //exp1(piBasClient, piBasServer, db);

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    LogSrcClient logSrcClient(piBasClient);
    LogSrcServer logSrcServer(piBasServer);
    exp1(logSrcClient, logSrcServer, db);

    std::cout << "\n---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    LogSrciClient logSrciClient(piBasClient);
    LogSrciServer logSrciServer(piBasServer);
    exp1(logSrciClient, logSrciServer, db);

    // experiment 2: fixed range size and vary db sizes
}
