#include <chrono>
#include <cmath>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"
#include "sda.h"

template <class DbDoc = IdOp, class DbKw = Kw>
Db<DbDoc, DbKw> createDb(int dbSize, bool isDataSkewed) {
    Db<DbDoc, DbKw> db;
    if (isDataSkewed) {
        // two uniq keywords, each making up half the dataset, with one being 0 and the other being the max
        // this means half of them will be returned as false positives on a [1, n - 1] query if the root node is the SRC
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
    sse.setup(KEY_LEN, db);
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
        std::cout << "Search time (size 2^" << (int)log2(query.size()) << "): " << searchElapsed.count() * 1000 << " ms"
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
        sse.setup(KEY_LEN, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size 2^" << (int)log2(dbSize) << "): " << setupElapsed.count() * 1000 << " ms"
                << std::endl;

        // query
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << (int)log2(dbSize) << "): " << searchElapsed.count() * 1000 << " ms"
                << std::endl;
    }
    std::cout << std::endl;
}

template <class DbDoc, class DbKw>
void exp3(ISse<DbDoc, DbKw>& sse, int dbSize) {
    Db<DbDoc, DbKw> db = createDb(dbSize, true);

    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_LEN, db);
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
    PiBasResHiding piBasResHiding;
    LogSrc<PiBas> logSrc;
    LogSrci<PiBas> logSrci;
    Sda<PiBasResHiding<>> sdaPiBas;
    Sda<LogSrc<PiBasResHiding>> sdaLogSrc;
    Sda<LogSrci<PiBasResHiding>> sdaLogSrci;

    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    int maxDbSize = pow(2, maxDbSizeExp);
    std::cout << std::endl;

    // experiment 1

    std::cout << "-------------------- Experiment 1 --------------------" << std::endl;
    std::cout << "DB size  : 2^"      << maxDbSizeExp << " (16 times smaller for SDa)" << std::endl;
    std::cout << "Query    : varied " << std::endl;
    std::cout << "Data skew: no"      << std::endl;
    std::cout << std::endl;

    std::cout << "---------- PiBas ----------" << std::endl;
    std::cout << std::endl;
    exp1(piBas, maxDbSize);
    piBas.setup(KEY_LEN, Db {}); // hopefully clear memory asap

    std::cout << "---------- PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(piBasResHiding, maxDbSize);
    piBasResHiding.setup(KEY_LEN, Db {});

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrc, maxDbSize);
    logSrc.setup(KEY_LEN, Db {});

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrci, maxDbSize);
    logSrci.setup(KEY_LEN, Db {});

    std::cout << "---------- SDa with PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaPiBas, maxDbSize / 16);
    sdaPiBas.setup(KEY_LEN, Db {});

    std::cout << "---------- SDa with Log-SRC (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrc, maxDbSize / 16);
    sdaLogSrc.setup(KEY_LEN, Db {});

    std::cout << "---------- SDa with Log-SRC-i (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrci, maxDbSize / 16);
    sdaLogSrci.setup(KEY_LEN, Db {});

    // experiment 2

    std::cout << "-------------------- Experiment 2 --------------------" << std::endl;
    std::cout << "DB size  : varied" << std::endl;
    std::cout << "Query    : 0-3"    << std::endl;
    std::cout << "Data skew: no"     << std::endl;
    std::cout << std::endl;

    std::cout << "---------- PiBas ----------" << std::endl;
    std::cout << std::endl;
    exp2(piBas, maxDbSize / 2);
    piBas.setup(KEY_LEN, Db {});

    std::cout << "---------- PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(piBasResHiding, maxDbSize / 2);
    piBasResHiding.setup(KEY_LEN, Db {});

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrc, maxDbSize / 2);
    logSrc.setup(KEY_LEN, Db {});

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrci, maxDbSize / 2);
    logSrci.setup(KEY_LEN, Db {});

    std::cout << "---------- SDa with PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaPiBas, maxDbSize / 2);
    sdaPiBas.setup(KEY_LEN, Db {});

    std::cout << "---------- SDa with Log-SRC (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrc, maxDbSize / 2);
    sdaLogSrc.setup(KEY_LEN, Db {});
    
    std::cout << "---------- SDa with Log-SRC-i (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrci, maxDbSize / 2);
    sdaLogSrci.setup(KEY_LEN, Db {});
    
    // experiment 3

    std::cout << "-------------------- Experiment 3 --------------------" << std::endl;
    std::cout << "DB size  : 2^"     << maxDbSizeExp << std::endl;
    std::cout << "Query    : varied" << std::endl;
    std::cout << "Data skew: yes"    << std::endl;
    std::cout << std::endl;

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrc, maxDbSize);
    logSrc.setup(KEY_LEN, Db {});

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrci, maxDbSize);
    logSrci.setup(KEY_LEN, Db {});
}
