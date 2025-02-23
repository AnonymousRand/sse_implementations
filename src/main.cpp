#include <chrono>
#include <cmath>
#include <random>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"

static std::random_device dev;
static std::mt19937 rng(dev());

template <class DbDoc = IdOp, class DbKw = Kw>
Db<DbDoc, DbKw> createDb(int dbSize, bool isDataSkewed) {
    Db<DbDoc, DbKw> db;
    if (isDataSkewed) {
        // two unique keywords, each making up half the dataset, with one being 0 and the other being the max
        // this means half of them will be returned as false positives on the penultimate query
        int kw1 = 0;
        int kw2 = dbSize - 1;

        for (int i = 0; i < dbSize / 2; i++) {
            db.push_back(std::pair {DbDoc(i), Range<DbKw> {kw1, kw1}});
        }
        for (int i = dbSize / 2; i < dbSize; i++) {
            db.push_back(std::pair {DbDoc(i), Range<DbKw> {kw2, kw2}});
        }
    } else {
        for (int i = 0; i < dbSize; i++) {
            db.push_back(std::pair {DbDoc(i), Range<DbKw> {i, i}});
        }
    }
    return db;
}

template <class DbDoc, class DbKw>
void exp1(ISse<DbDoc, DbKw>& sse, int dbSize) {
    Db<DbDoc, DbKw> db = createDb(dbSize, false);
    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_SIZE, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;
    std::cout << std::endl;

    // search
    for (int i = 0; i <= (int)log2(dbSize); i++) {
        KwRange query {0, (int)pow(2, i) - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size " << query.size() << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;
}

template <class DbDoc, class DbKw>
void exp2(ISse<DbDoc, DbKw>& sse, int maxDbSize) {
    Range<DbKw> query {0, 3};
    for (int i = 2; i <= (int)log2(maxDbSize); i++) {
        int dbSize = (int)pow(2, i);
        Db<DbDoc, DbKw> db = createDb(dbSize, false);

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_SIZE, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size " << dbSize << "): " << setupElapsed.count() * 1000 << " ms" << std::endl;

        // query
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size " << dbSize << "): " << searchElapsed.count() * 1000 << " ms" << std::endl;
    }
    std::cout << std::endl;
}

template <class DbDoc, class DbKw>
void exp3(ISse<DbDoc, DbKw>& sse, int dbSize) {
    Db<DbDoc, DbKw> db = createDb(dbSize, true);
    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_SIZE, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;

    // search
    KwRange query {1, dbSize - 1};
    auto searchStartTime = std::chrono::high_resolution_clock::now();
    sse.search(query);
    auto searchEndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
    std::cout << "Search time: " << searchElapsed.count() * 1000 << " ms" << std::endl;
    std::cout << std::endl;
}

int main() {
    PiBas piBas;
    PiBas<> logSrcUnderly;
    LogSrc<> logSrc(logSrcUnderly);
    PiBas<SrciDb1Doc<>, Kw> logSrciUnderly1;
    PiBas<IdOp, Id> logSrciUnderly2;
    LogSrci<> logSrci(logSrciUnderly1, logSrciUnderly2);

    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    int maxDbSize = pow(2, maxDbSizeExp);

    // experiment 1

    std::cout << "---------- Experiment 1 for PiBas ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(piBas, maxDbSize);
    piBas.setup(KEY_SIZE, Db<>());

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrc, maxDbSize);
    logSrc.setup(KEY_SIZE, Db<>());

    std::cout << "---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrci, maxDbSize);
    logSrci.setup(KEY_SIZE, Db<>());

    // experiment 2

    std::cout << "---------- Experiment 2 for PiBas ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : 0-3"         << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(piBas, maxDbSize / 2);    // halved db sizes due to heavy memory usage in experiment 2
    piBas.setup(KEY_SIZE, Db<>()); // hopefully clear memory asap
    
    std::cout << "---------- Experiment 2 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : 0-3"         << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrc, maxDbSize / 2);
    logSrc.setup(KEY_SIZE, Db<>());

    std::cout << "---------- Experiment 2 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : 0-3"         << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrci, maxDbSize / 2);
    logSrci.setup(KEY_SIZE, Db<>());

    // experiment 3

    std::cout << "---------- Experiment 3 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp3(logSrc, maxDbSize);
    logSrc.setup(KEY_SIZE, Db<>());

    std::cout << "---------- Experiment 3 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp3(logSrci, maxDbSize);
    logSrci.setup(KEY_SIZE, Db<>());
}
