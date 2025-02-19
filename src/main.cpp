#include <chrono>
#include <cmath>
#include <random>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"

static std::random_device dev;
static std::mt19937 rng(dev());

Db<> createDb(int dbSize, bool isDataSkewed) {
    Db<> db;
    if (isDataSkewed) {
        std::normal_distribution dist((dbSize - 1) / 2.0, dbSize / 1000.0);
        for (int i = 0; i < dbSize; i++) {
            Kw kw;
            do {
                kw = (int)dist(rng);
            } while (kw >= dbSize);
            db.push_back(std::make_tuple(Id(i), KwRange {kw, kw}));
        }
    } else {
        for (int i = 0; i < dbSize; i++) {
            Kw kw = i;
            db.push_back(std::make_tuple(Id(i), KwRange {kw, kw}));
        }
    }
    sortDb(db);
    return db;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp1(LogSrcClient<UnderlyingClient>& client, LogSrcServer<UnderlyingServer>& server, Db<> db, int dbSize) {
    // setup
    ustring key = client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    auto buildIndexStart = std::chrono::high_resolution_clock::now();
    EncInd encInd;
    client.buildIndex(key, db, encInd);
    auto buildIndexEnd = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> buildIndexElapsed = buildIndexEnd - buildIndexStart;
    std::cout << "Build index execution time: " << buildIndexElapsed.count() << " s" << std::endl;
    std::cout << std::endl;

    // query
    std::chrono::duration<double> trpdrElapsed, searchElapsed;
    std::cout << "Querying..." << std::endl;
    int queryCount = 0;
    for (int i = 0; i <= (int)log2(dbSize); i++) {
        queryCount++;
        KwRange query {0, (int)pow(2, i) - 1};

        auto trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken = client.trpdr(key, query);
        auto trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed = trpdrEndTime - trpdrStartTime;
        std::cout << "Trpdr execution time (size " << query.size() << "): " << trpdrElapsed.count() * 1000 << " ms"
                  << std::endl;

        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Id> results;
        server.search(encInd, queryToken, results);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search execution time (size " << query.size() << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << std::endl;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp1(LogSrciClient<UnderlyingClient>& client, LogSrciServer<UnderlyingServer>& server, Db<> db, int dbSize) {
    // setup
    std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    auto buildIndexStart = std::chrono::high_resolution_clock::now();
    std::pair<EncInd, EncInd> encInds;
    client.buildIndex(keys, db, encInds);
    auto buildIndexEnd = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> buildIndexElapsed = buildIndexEnd - buildIndexStart;
    std::cout << "Build index execution time: " << buildIndexElapsed.count() << " s" << std::endl;
    std::cout << std::endl;

    // query
    std::chrono::duration<double> trpdrElapsed, searchElapsed;
    std::cout << "Querying..." << std::endl;
    int queryCount = 0;
    for (int i = 0; i <= (int)log2(dbSize); i++) {
        queryCount++;
        KwRange query {0, (int)pow(2, i) - 1};

        auto trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken1 = client.trpdr1(keys.first, query);
        auto trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed = trpdrEndTime - trpdrStartTime;

        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<SrciDb1Doc> results1;
        server.search1(encInds.first, queryToken1, results1);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed = searchEndTime - searchStartTime;

        trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed += trpdrEndTime - trpdrStartTime;
        std::cout << "Trpdr execution time (size " << query.size() << "): " << trpdrElapsed.count() * 1000 << " ms"
                  << std::endl;

        searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Id> results2;
        server.search2(encInds.second, queryToken2, results2);
        searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed += searchEndTime - searchStartTime;
        std::cout << "Search execution time (size " << query.size() << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp2(
    LogSrcClient<UnderlyingClient>& client, LogSrcServer<UnderlyingServer>& server,
    KwRange query, int maxDbSize, bool isDataSkewed
) {
    std::chrono::duration<double> trpdrElapsed, searchElapsed;
    int queryCount = 0;
    for (int i = 1; i <= (int)log2(maxDbSize); i++) {
        queryCount++;
        int dbSize = (int)pow(2, i);
        Db<> db = createDb(dbSize, isDataSkewed);

        // setup
        ustring key = client.setup(KEY_SIZE);
        auto buildIndexStart = std::chrono::high_resolution_clock::now();
        EncInd encInd;
        client.buildIndex(key, db, encInd);
        auto buildIndexEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> buildIndexElapsed = buildIndexEnd - buildIndexStart;
        std::cout << "Build index execution time (size " << dbSize << "): "
                  << buildIndexElapsed.count() * 1000 << " ms" << std::endl;

        // query
        auto trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken = client.trpdr(key, query);
        auto trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed = trpdrEndTime - trpdrStartTime;
        std::cout << "Trpdr execution time: " << trpdrElapsed.count() * 1000 << " ms" << std::endl;

        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Id> results;
        server.search(encInd, queryToken, results);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search execution time: " << searchElapsed.count() * 1000 << " ms" << std::endl;

    }
    std::cout << std::endl;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp2(
    LogSrciClient<UnderlyingClient>& client, LogSrciServer<UnderlyingServer>& server,
    KwRange query, int maxDbSize, bool isDataSkewed
) {
    std::chrono::duration<double> trpdrElapsed, searchElapsed;
    int queryCount = 0;
    for (int i = 1; i <= (int)log2(maxDbSize); i++) {
        queryCount++;
        int dbSize = (int)pow(2, i);
        Db<> db = createDb(dbSize, isDataSkewed);

        // setup
        std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
        auto buildIndexStart = std::chrono::high_resolution_clock::now();
        std::pair<EncInd, EncInd> encInds;
        client.buildIndex(keys, db, encInds);
        auto buildIndexEnd = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> buildIndexElapsed = buildIndexEnd - buildIndexStart;
        std::cout << "Build index execution time (size " << dbSize << "): "
                  << buildIndexElapsed.count() * 1000 << " ms" << std::endl;

        // query
        auto trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken1 = client.trpdr1(keys.first, query);
        auto trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed = trpdrEndTime - trpdrStartTime;

        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<SrciDb1Doc> results1;
        server.search1(encInds.first, queryToken1, results1);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed = searchEndTime - searchStartTime;

        trpdrStartTime = std::chrono::high_resolution_clock::now();
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        trpdrEndTime = std::chrono::high_resolution_clock::now();
        trpdrElapsed += trpdrEndTime - trpdrStartTime;
        std::cout << "Trpdr execution time: " << trpdrElapsed.count() * 1000 << " ms"
                  << std::endl;

        searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Id> results2;
        server.search2(encInds.second, queryToken2, results2);
        searchEndTime = std::chrono::high_resolution_clock::now();
        searchElapsed += searchEndTime - searchStartTime;
        std::cout << "Search execution time: " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    PiBasClient piBasClient;
    PiBasServer piBasServer;
    LogSrcClient logSrcClient(piBasClient);
    LogSrcServer logSrcServer(piBasServer);
    LogSrciClient logSrciClient(piBasClient);
    LogSrciServer logSrciServer(piBasServer);
    int maxDbSize = pow(2, 20);

    // experiment 1
    Db<> db = createDb(maxDbSize, false);

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrcClient, logSrcServer, db, maxDbSize);
    logSrcClient = LogSrcClient(piBasClient); // reassign immediately to hopefully reclaim memory
    logSrcServer = LogSrcServer(piBasServer);

    std::cout << "---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrciClient, logSrciServer, db, maxDbSize);
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);

    // experiment 1.5
    db = createDb(maxDbSize, true);

    std::cout << "---------- Experiment 1.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp1(logSrcClient, logSrcServer, db, maxDbSize);
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);

    std::cout << "---------- Experiment 1.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp1(logSrciClient, logSrciServer, db, maxDbSize);
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);

    // experiment 2
    std::uniform_int_distribution dist(0, maxDbSize);
    Kw lowerEnd = dist(rng);
    Kw upperEnd;
    do {
       upperEnd = dist(rng);
    } while (upperEnd < lowerEnd);
    KwRange query = KwRange {lowerEnd, upperEnd};
    
    std::cout << "---------- Experiment 2 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrcClient, logSrcServer, query, maxDbSize, false);
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);

    std::cout << "---------- Experiment 2 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrciClient, logSrciServer, query, maxDbSize, false);
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);

    // experiment 2.5
    std::cout << "---------- Experiment 2.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp2(logSrcClient, logSrcServer, query, maxDbSize, true);
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);

    std::cout << "---------- Experiment 2.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp2(logSrciClient, logSrciServer, query, maxDbSize, true);
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
}
