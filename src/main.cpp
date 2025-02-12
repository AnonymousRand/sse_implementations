#include <chrono>
#include <cmath>
#include <random>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"

static std::random_device dev;
static std::mt19937 rng(dev());

template <typename UnderlyingClient, typename UnderlyingServer>
void exp1(LogSrcClient<UnderlyingClient>& client, LogSrcServer<UnderlyingServer>& server, Db<> db, int dbSize) {
    auto startTime = std::chrono::high_resolution_clock::now();

    ustring key = client.setup(KEY_SIZE);
    auto setupSplit = std::chrono::high_resolution_clock::now();
    std::cout << "Building index..." << std::endl;
    EncInd encInd = client.buildIndex(key, db);
    auto buildIndexSplit = std::chrono::high_resolution_clock::now();

    std::cout << "Querying..." << std::endl;
    int queryCount = 0;
    // assume keywords are less than `dbSize`
    for (int i = 0; i < (int)log2(dbSize); i++) {
        queryCount++;
        KwRange query {0, (int)pow(2, i)};
        QueryToken queryToken = client.trpdr(key, query);
        std::vector<Id> results = server.search(encInd, queryToken);
    }
    auto querySplit = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> totalElapsed      = querySplit - startTime;
    std::chrono::duration<double> buildIndexElapsed = buildIndexSplit - setupSplit;
    std::chrono::duration<double> queryElapsed      = querySplit - buildIndexSplit;
    std::cout << "\nExecution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "BuildIndex: " << buildIndexElapsed.count()         << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
    std::cout << std::endl;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp1(LogSrciClient<UnderlyingClient>& client, LogSrciServer<UnderlyingServer>& server, Db<> db, int dbSize) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
    auto setupSplit = std::chrono::high_resolution_clock::now();
    std::cout << "Building index..." << std::endl;
    std::pair<EncInd, EncInd> encInds = client.buildIndex(keys, db);
    auto buildIndexSplit = std::chrono::high_resolution_clock::now();

    std::cout << "Querying..." << std::endl;
    int queryCount = 0;
    for (int i = 0; i < (int)log2(dbSize); i++) {
        queryCount++;
        KwRange query {0, (int)pow(2, i)};
        QueryToken queryToken1 = client.trpdr1(keys.first, query);
        std::vector<SrciDb1Doc> results1 = server.search1(encInds.first, queryToken1);
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        std::vector<Id> results2 = server.search2(encInds.second, queryToken2);
    }
    auto querySplit = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> totalElapsed      = querySplit - startTime;
    std::chrono::duration<double> buildIndexElapsed = buildIndexSplit - setupSplit;
    std::chrono::duration<double> queryElapsed      = querySplit - buildIndexSplit;
    std::cout << "\nExecution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "BuildIndex: " << buildIndexElapsed.count()         << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
    std::cout << std::endl;
}

Db<> createDb(int dbSize, bool isDataSkewed) {
    Db<> db;
    if (isDataSkewed) {
        std::normal_distribution dist((dbSize - 1) / 2.0, dbSize / 100.0);
        for (int i = 0; i < dbSize; i++) {
            Kw kw;
            do {
                kw = (int)dist(rng);
            } while (kw >= dbSize);
            db.push_back(Doc {Id(i), KwRange {kw, kw}});
        }
    } else {
        for (int i = 0; i < dbSize; i++) {
            Kw kw = i;
            db.push_back(Doc {Id(i), KwRange {kw, kw}});
        }
    }
    sortDb(db);
    return db;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp2(
    LogSrcClient<UnderlyingClient>& client, LogSrcServer<UnderlyingServer>& server,
    KwRange query, int maxDbSize, bool isDataSkewed
) {
    std::chrono::duration<double> totalElapsed;
    std::chrono::duration<double> queryElapsed;

    int queryCount = 0;
    for (int i = 1; i <= (int)log2(maxDbSize); i++) {
        queryCount++;
        Db<> db = createDb((int)pow(2, i), isDataSkewed);
        auto startTime = std::chrono::high_resolution_clock::now();

        ustring key = client.setup(KEY_SIZE);
        EncInd encInd = client.buildIndex(key, db);
        auto buildIndexSplit = std::chrono::high_resolution_clock::now();

        QueryToken queryToken = client.trpdr(key, query);
        std::vector<Id> results = server.search(encInd, queryToken);
        auto querySplit = std::chrono::high_resolution_clock::now();

        totalElapsed += querySplit - startTime;
        queryElapsed += querySplit - buildIndexSplit;
    }

    std::cout << "\nExecution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
    std::cout << std::endl;
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp2(
    LogSrciClient<UnderlyingClient>& client, LogSrciServer<UnderlyingServer>& server,
    KwRange query, int maxDbSize, bool isDataSkewed
) {
    std::chrono::duration<double> totalElapsed;
    std::chrono::duration<double> queryElapsed;

    int queryCount = 0;
    for (int i = 1; i <= (int)log2(maxDbSize); i++) {
        queryCount++;
        Db<> db = createDb((int)pow(2, i), isDataSkewed);
        auto startTime = std::chrono::high_resolution_clock::now();

        std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
        std::pair<EncInd, EncInd> encInds = client.buildIndex(keys, db);
        auto buildIndexSplit = std::chrono::high_resolution_clock::now();

        QueryToken queryToken1 = client.trpdr1(keys.first, query);
        std::vector<SrciDb1Doc> results1 = server.search1(encInds.first, queryToken1);
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        std::vector<Id> results2 = server.search2(encInds.second, queryToken2);
        auto querySplit = std::chrono::high_resolution_clock::now();

        totalElapsed += querySplit - startTime;
        queryElapsed += querySplit - buildIndexSplit;
    }

    std::cout << "Execution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
    std::cout << std::endl;
}

int main() {
    PiBasClient piBasClient;
    PiBasServer piBasServer;
    LogSrcClient logSrcClient(piBasClient);
    LogSrcServer logSrcServer(piBasServer);
    LogSrciClient logSrciClient(piBasClient);
    LogSrciServer logSrciServer(piBasServer);
    int maxDbSize = pow(2, 18);

    // experiment 1
    Db<> db = createDb(maxDbSize, false);

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : " << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no" << std::endl;
    std::cout << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp1(logSrcClient, logSrcServer, db, maxDbSize);

    std::cout << "---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : " << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no" << std::endl;
    std::cout << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp1(logSrciClient, logSrciServer, db, maxDbSize);

    // experiment 1.5
    db = createDb(maxDbSize, true);

    std::cout << "---------- Experiment 1.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : " << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes" << std::endl;
    std::cout << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp1(logSrcClient, logSrcServer, db, maxDbSize);

    std::cout << "---------- Experiment 1.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : " << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes" << std::endl;
    std::cout << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp1(logSrciClient, logSrciServer, db, maxDbSize);

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
    std::cout << "Query    : " << query << std::endl;
    std::cout << "Data skew: no" << std::endl;
    std::cout << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp2(logSrcClient, logSrcServer, query, maxDbSize, false);

    std::cout << "---------- Experiment 2 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : " << query << std::endl;
    std::cout << "Data skew: no" << std::endl;
    std::cout << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp2(logSrciClient, logSrciServer, query, maxDbSize, false);

    // experiment 2.5
    std::cout << "---------- Experiment 2.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : " << query << std::endl;
    std::cout << "Data skew: yes" << std::endl;
    std::cout << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp2(logSrcClient, logSrcServer, query, maxDbSize, true);

    std::cout << "---------- Experiment 2.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : " << query << std::endl;
    std::cout << "Data skew: yes" << std::endl;
    std::cout << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp2(logSrciClient, logSrciServer, query, maxDbSize, true);
}
