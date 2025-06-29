#include <chrono>
#include <cmath>

#include "log_src.h"
#include "log_src_i.h"
#include "pi_bas.h"
#include "sda.h"


Db<> createDb(ulong dbSize, bool isDataSkewed) {
    Db<> db;
    if (dbSize == 0) {
        return db;
    }
    if (isDataSkewed) {
        // two unique keywords, with all but one being 0 and the other being the max
        // thus all but one doc will be returned as false positives on a [1, n - 1] query (if the root node is the SRC)
        ulong kw1 = 0;
        ulong kw2 = dbSize - 1;

        for (ulong i = 0; i < dbSize - 1; i++) {
            db.push_back(DbEntry {Doc(i, kw1, Op::INS), Range<Kw> {kw1, kw1}});
        }
        db.push_back(DbEntry {Doc(dbSize - 1, kw2, Op::INS), Range<Kw> {kw2, kw2}});
    } else {
        for (ulong i = 0; i < dbSize; i++) {
            // make keywords and ids inversely proportional to test sorting of Log-SRC-i's second index
            Kw kw = dbSize - i + 1;
            db.push_back(DbEntry {Doc(i, kw, Op::INS), Range<Kw> {kw, kw}});
        }
    }
    return db;
}


// experiment for debugging with fixed query and printed results
void expDebug(ISse<>& sse, ulong dbSize, Range<Kw> query) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, false);

    // setup
    sse.setup(KEY_LEN, db);

    // search
    std::vector<Doc> results = sse.search(query);
    std::cout << "Results (id,kw,op):" << std::endl;
    for (Doc result : results) {
        Range<Kw> kw;
        for (DbEntry<> dbEntry : db) {
            if (dbEntry.first == result) {
                kw = dbEntry.second;
                break;
            }
        }
        std::cout << result << " with keyword " << kw << std::endl;
    }
    std::cout << std::endl;

    // hopefully clear memory asap
    sse.clear();
}


void exp1(ISse<>& sse, ulong dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createDb(dbSize, false);

    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_LEN, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;
    std::cout << std::endl;

    // search
    for (ulong i = 0; i <= log2(dbSize); i++) {
        Range<Kw> query {0, (ulong)pow(2, i) - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << log2(query.size()) << "): " << searchElapsed.count() * 1000
                  << " ms" << std::endl;
    }
    std::cout << std::endl;

    // hopefully clear memory asap
    sse.clear();
}


void exp2(ISse<>& sse, ulong maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};

    for (ulong i = 2; i <= log2(maxDbSize); i++) {
        ulong dbSize = pow(2, i);
        Db<> db = createDb(dbSize, false);

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_LEN, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size 2^" << log2(dbSize) << "): " << setupElapsed.count() * 1000 << " ms"
                  << std::endl;

        // search
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << log2(dbSize) << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;

    // hopefully clear memory asap
    sse.clear();
}


void exp3(ISse<>& sse, ulong maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    for (ulong i = 2; i <= log2(maxDbSize); i++) {
        ulong dbSize = pow(2, i);
        Db<> db = createDb(dbSize, true);

        // setup
        auto setupStart = std::chrono::high_resolution_clock::now();
        sse.setup(KEY_LEN, db);
        auto setupEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
        std::cout << "Setup time (size 2^" << log2(dbSize) << "): " << setupElapsed.count() * 1000 << " ms"
                  << std::endl;

        // search
        Range<Kw> query {1, dbSize - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << log2(dbSize) << "): " << searchElapsed.count() * 1000 << " ms"
                  << std::endl;
    }
    std::cout << std::endl;

    // hopefully clear memory asap
    sse.clear();
}


int main() {
    ulong maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    ulong maxDbSize = pow(2, maxDbSizeExp);

    std::string encIndTypeStr;
    EncIndType encIndType;
    do {
        std::cout << "Encrypted indexes stored on RAM or disk? [r/d]: ";
        std::cin >> encIndTypeStr;
    } while (encIndTypeStr != "r" && encIndTypeStr != "d");
    std::cout << std::endl;
    if (encIndTypeStr == "r") {
        encIndType = EncIndType::RAM;
    } else {
        encIndType = EncIndType::DISK;
    }
    std::cout << std::endl;

    PiBas<> piBas(encIndType);
    PiBasResHiding<> piBasResHiding(encIndType);
    LogSrc<PiBas> logSrc(encIndType);
    LogSrcI<PiBas> logSrcI(encIndType);
    Sda<PiBasResHiding<>> sdaPiBas(encIndType);
    Sda<LogSrc<PiBasResHiding>> sdaLogSrc(encIndType);
    Sda<LogSrcI<PiBasResHiding>> sdaLogSrcI(encIndType);

    /////////////////////////// debugging experiment ///////////////////////////

    Range<Kw> query {3, 5};

    std::cout << "------------------------- Debugging Experiment -------------------------" << std::endl;
    std::cout << "DB size  : 2^" << maxDbSizeExp << std::endl;
    std::cout << "Query    : "   << query << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------- PiBas ----------" << std::endl;
    std::cout << std::endl;
    expDebug(piBas, maxDbSize, query);

    std::cout << "---------- PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    expDebug(piBasResHiding, maxDbSize, query);

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrc, maxDbSize, query);

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcI, maxDbSize, query);

    std::cout << "---------- SDa with PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaPiBas, maxDbSize, query);

    std::cout << "---------- SDa with Log-SRC (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrc, maxDbSize, query);

    std::cout << "---------- SDa with Log-SRC-i (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcI, maxDbSize, query);

    /////////////////////////////// experiment 1 ///////////////////////////////

    std::cout << "------------------------- Experiment 1 -------------------------" << std::endl;
    std::cout << "DB size  : 2^"     << maxDbSizeExp << " (2^" << (maxDbSizeExp >= 5 ? maxDbSizeExp - 5 : 0)
              << " for SDa)" << std::endl;
    std::cout << "Query    : varied" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------- PiBas ----------" << std::endl;
    std::cout << std::endl;
    exp1(piBas, maxDbSize);

    std::cout << "---------- PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(piBasResHiding, maxDbSize);

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrc, maxDbSize);

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcI, maxDbSize);

    std::cout << "---------- SDa with PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaPiBas, maxDbSize / 32);

    std::cout << "---------- SDa with Log-SRC (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrc, maxDbSize / 32);

    std::cout << "---------- SDa with Log-SRC-i (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcI, maxDbSize / 32);

    /////////////////////////////// experiment 2 ///////////////////////////////

    std::cout << "------------------------- Experiment 2 -------------------------" << std::endl;
    std::cout << "DB size  : varied, up to 2^" << maxDbSizeExp << " (2^" << (maxDbSizeExp >= 5 ? maxDbSizeExp - 5 : 0)
              << " for SDa)" << std::endl;
    std::cout << "Query    : 0-3"              << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------- PiBas ----------" << std::endl;
    std::cout << std::endl;
    exp2(piBas, maxDbSize);

    std::cout << "---------- PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(piBasResHiding, maxDbSize);

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrc, maxDbSize);

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcI, maxDbSize);

    std::cout << "---------- SDa with PiBas (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaPiBas, maxDbSize / 32);

    std::cout << "---------- SDa with Log-SRC (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrc, maxDbSize / 32);
    
    std::cout << "---------- SDa with Log-SRC-i (result-hiding) ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcI, maxDbSize / 32);
    
    /////////////////////////////// experiment 3 ///////////////////////////////

    std::cout << "------------------------- Experiment 3 -------------------------" << std::endl;
    std::cout << "DB size  : 2^"                         << maxDbSizeExp << std::endl;
    std::cout << "Query    : incurs 50% false positives" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------- Log-SRC ----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrc, maxDbSize);

    std::cout << "---------- Log-SRC-i ----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcI, maxDbSize);
}
