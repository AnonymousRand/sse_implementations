// temp compilation `g++ main.cpp sse.cpp range_sse.cpp pi_bas.cpp log_src.cpp log_srci.cpp util/*.cpp -lcrypto -o a`
#include <chrono>
#include <cmath>
#include <random>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"
#include "sse.h"

void exp1(ISseClient<ustring, EncInd>& client, ISseServer& server, Db<> db, int dbSize) {
    auto startTime = std::chrono::high_resolution_clock::now();

    ustring key = client.setup(KEY_SIZE);
    auto setupSplit = std::chrono::high_resolution_clock::now();
    std::cout << "Building index..." << std::endl;
    EncInd encInd = client.buildIndex(key, db);
    auto buildIndexSplit = std::chrono::high_resolution_clock::now();

    std::cout << "Querying..." << std::endl;
    int queryCount = 0;
    // assume keywords are less than `dbSize`
    for (int end = 0; end < dbSize; end++) {
        queryCount++;
        KwRange query {0, end};
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
    for (int end = 0; end < dbSize; end++) {
        queryCount++;
        KwRange query {0, end};
        QueryToken queryToken1 = client.trpdr(keys.first, query);
        std::vector<SrciDb1Doc> results1 = server.search1(encInds.first, queryToken1);
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        std::vector<Id> results2 = server.search(encInds.second, queryToken2);
    }
    auto querySplit = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> totalElapsed      = querySplit - startTime;
    std::chrono::duration<double> buildIndexElapsed = buildIndexSplit - setupSplit;
    std::chrono::duration<double> queryElapsed      = querySplit - buildIndexSplit;
    std::cout << "\nExecution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "BuildIndex: " << buildIndexElapsed.count()         << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
}

Db<> exp2CreateDb(int dbSize, bool useUniformData) {
    Db<> db;
    if (useUniformData) {
        for (int i = 0; i < dbSize; i++) {
            Kw kw = i;
            db.push_back(Doc {Id(i), KwRange {kw, kw}});
        }
    } else {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::normal_distribution dist((dbSize - 1) / 2.0, dbSize / 100.0);
        for (int i = 0; i < dbSize; i++) {
            Kw kw;
            do {
                kw = (int)dist(rng);
            } while (kw >= dbSize);
            db.push_back(Doc {Id(i), KwRange {kw, kw}});
        }
    }
    return db;
}

void exp2(ISseClient<ustring, EncInd>& client, ISseServer& server, KwRange query, int maxDbSize, bool useUniformData) {
    std::chrono::duration<double> totalElapsed;
    std::chrono::duration<double> queryElapsed;

    int queryCount = 0;
    for (int dbSize = 1; dbSize <= maxDbSize; dbSize++) {
        Db<> db = exp2CreateDb(dbSize, useUniformData);
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
}

template <typename UnderlyingClient, typename UnderlyingServer>
void exp2(
    LogSrciClient<UnderlyingClient>& client, LogSrciServer<UnderlyingServer>& server, KwRange query,
    int maxDbSize, bool useUniformData
) {
    std::chrono::duration<double> totalElapsed;
    std::chrono::duration<double> queryElapsed;

    int queryCount = 0;
    for (int dbSize = 1; dbSize <= maxDbSize; dbSize++) {
        Db<> db = exp2CreateDb(dbSize, useUniformData);
        auto startTime = std::chrono::high_resolution_clock::now();

        std::pair<ustring, ustring> keys = client.setup(KEY_SIZE);
        std::pair<EncInd, EncInd> encInds = client.buildIndex(keys, db);
        auto buildIndexSplit = std::chrono::high_resolution_clock::now();

        QueryToken queryToken1 = client.trpdr(keys.first, query);
        std::vector<SrciDb1Doc> results1 = server.search1(encInds.first, queryToken1);
        QueryToken queryToken2 = client.trpdr2(keys.second, query, results1);
        std::vector<Id> results2 = server.search(encInds.second, queryToken2);
        auto querySplit = std::chrono::high_resolution_clock::now();

        totalElapsed += querySplit - startTime;
        queryElapsed += querySplit - buildIndexSplit;
    }

    std::cout << "\nExecution times:" << std::endl;
    std::cout << "Total     : " << totalElapsed.count()              << "s" << std::endl;
    std::cout << "Per query : " << queryElapsed.count() / queryCount << "s" << std::endl;
}

int main() {
    Db<> db;
    PiBasClient piBasClient;
    PiBasServer piBasServer;
    LogSrcClient logSrcClient(piBasClient);
    LogSrcServer logSrcServer(piBasServer);
    LogSrciClient logSrciClient(piBasClient);
    LogSrciServer logSrciServer(piBasServer);

    // experiment 1: uniform db of size 2^16 and varied query range sizes
    int dbSize = pow(2, 16);
    for (int i = 0; i < dbSize; i++) {
        Kw kw = i;
        db.push_back(Doc {Id(i), KwRange {kw, kw}});
    }
    sortDb(db);

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    std::cout << "DB of size " << dbSize << " with uniform data and varied query range sizes."
              << std::endl << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp1(logSrcClient, logSrcServer, db, dbSize);

    std::cout << "\n---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB of size " << dbSize << " with uniform data and varied query range sizes."
              << std::endl << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp1(logSrciClient, logSrciServer, db, dbSize);

    // experiment 1.5: skewed db of size 2^16 and varied query range sizes
    db.clear();
    std::random_device dev;
    std::mt19937 rng(dev());
    std::normal_distribution dist((dbSize - 1) / 2.0, dbSize / 100.0);
    for (int i = 0; i < dbSize; i++) {
        Kw kw;
        do {
            kw = (int)dist(rng);
        } while (kw >= dbSize);
        db.push_back(Doc {Id(i), KwRange {kw, kw}});
    }
    sortDb(db);

    std::cout << "---------- Experiment 1.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB of size " << dbSize << " with skewed data and varied query range sizes."
              << std::endl << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp1(logSrcClient, logSrcServer, db, dbSize);

    std::cout << "\n---------- Experiment 1.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB of size " << dbSize << " with skewed data and varied query range sizes."
              << std::endl << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp1(logSrciClient, logSrciServer, db, dbSize);

    // experiment 2: fixed (random) query range size and varied db sizes
    std::uniform_int_distribution dist2(0, dbSize);
    Kw lowerEnd = dist2(rng);
    Kw upperEnd;
    do {
       upperEnd = dist2(rng);
    } while (upperEnd < lowerEnd);
    KwRange query = KwRange {lowerEnd, upperEnd};
    
    std::cout << "---------- Experiment 2 for Log-SRC ----------" << std::endl;
    std::cout << "DB with varied size, uniform data, and query " << query << "." << std::endl << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp2(logSrcClient, logSrcServer, query, dbSize, true);

    std::cout << "\n---------- Experiment 2 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB with varied size, uniform data, and query " << query << "." << std::endl << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp2(logSrciClient, logSrciServer, query, dbSize, true);

    std::cout << "---------- Experiment 2.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB with varied size, uniform data, and query " << query << "." << std::endl << std::endl;
    logSrcClient = LogSrcClient(piBasClient);
    logSrcServer = LogSrcServer(piBasServer);
    exp2(logSrcClient, logSrcServer, query, dbSize, false);

    std::cout << "\n---------- Experiment 2.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB with varied size, uniform data, and query " << query << "." << std::endl << std::endl;
    logSrciClient = LogSrciClient(piBasClient);
    logSrciServer = LogSrciServer(piBasServer);
    exp2(logSrciClient, logSrciServer, query, dbSize, false);
}
