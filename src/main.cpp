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
        // at most two unique keywords, each making up half the dataset
        std::uniform_int_distribution dist(0, dbSize - 1);
        int kw1 = dist(rng);
        int kw2;
        do {
            kw2 = dist(rng);
        } while (kw2 < kw1); // make sure we are sorted for Log-SRC-i: larger ids must have larger kws (`kw2` >= `kw1`)

        for (int i = 0; i < dbSize / 2; i++) {
            db.push_back(std::pair {Id(i), KwRange {kw1, kw1}});
        }
        for (int i = dbSize / 2; i < dbSize; i++) {
            db.push_back(std::pair {Id(i), KwRange {kw2, kw2}});
        }
    } else {
        for (int i = 0; i < dbSize; i++) {
            Kw kw = i;
            db.push_back(std::pair {Id(i), KwRange {kw, kw}});
        }
    }
    return db;
}

void exp1(ISse& sse, Db<> db, int dbSize) {
    // setup
    std::cout << "Setting up..." << std::endl;
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_SIZE, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() << " s" << std::endl;
    std::cout << std::endl;

    // search
    std::cout << "Searching..." << std::endl;
    for (int i = 0; i <= (int)log2(dbSize); i++) {
        KwRange query {0, (int)pow(2, i) - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Doc> results = sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size " << query.size() << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;
}

void exp2(ISse& sse, KwRange query, int maxDbSize) {
    for (int i = 1; i <= (int)log2(maxDbSize); i++) {
        int dbSize = (int)pow(2, i);
        Db<> db = createDb(dbSize, false);
        std::cout << "DB size: " << dbSize << std::endl;

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_SIZE, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;

        // query
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        std::vector<Doc> results = sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time: " << searchElapsed.count() * 1000 << " ms" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    PiBas piBas;
    LogSrc<> logSrc(PiBas());
    LogSrci<> logSrci(PiBas());

    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    int maxDbSize = pow(2, maxDbSizeExp);

    // experiment 1
    Db<> db = createDb(maxDbSize, false);

    std::cout << "---------- Experiment 1 for PiBas ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(piBas, db, maxDbSize);

    std::cout << "---------- Experiment 1 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrc, db, maxDbSize);

    std::cout << "---------- Experiment 1 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp1(logSrci, db, maxDbSize);

    // experiment 1.5
    db = createDb(maxDbSize, true);

    std::cout << "---------- Experiment 1.5 for PiBas ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp1(piBas, db, maxDbSize);

    std::cout << "---------- Experiment 1.5 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp1(logSrc, db, maxDbSize);

    std::cout << "---------- Experiment 1.5 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : "            << maxDbSize << std::endl;
    std::cout << "Query    : varied size" << std::endl;
    std::cout << "Data skew: yes"         << std::endl;
    std::cout << std::endl;
    exp1(logSrci, db, maxDbSize);

    // experiment 2
    std::uniform_int_distribution dist(0, maxDbSize);
    Kw lowerEnd = dist(rng);
    Kw upperEnd;
    do {
       upperEnd = dist(rng);
    } while (upperEnd < lowerEnd);
    KwRange query = KwRange {lowerEnd, upperEnd};

    std::cout << "---------- Experiment 2 for PiBas ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(piBas, query, maxDbSize);
    
    std::cout << "---------- Experiment 2 for Log-SRC ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrc, query, maxDbSize);

    std::cout << "---------- Experiment 2 for Log-SRC-i ----------" << std::endl;
    std::cout << "DB size  : varied size" << std::endl;
    std::cout << "Query    : "            << query << std::endl;
    std::cout << "Data skew: no"          << std::endl;
    std::cout << std::endl;
    exp2(logSrci, query, maxDbSize);
}
