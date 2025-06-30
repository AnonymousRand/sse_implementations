#include <chrono>
#include <cmath>

#include "log_src.h"
#include "log_src_i.h"
#include "pi_bas.h"
#include "sda.h"


Db<> createUniformDb(long dbSize, bool reverseKwOrder, bool hasDeletions) {
    Db<> db;
    if (dbSize == 0) {
        return db;
    }

    if (hasDeletions) {
        for (long i = 0; i < dbSize - 1; i++) {
            // if `reverseKwOrder`, make keywords and ids inversely proportional to test sorting of Log-SRC-i's index 2
            // `+1` to test the case where the smallest keyword is greater than `0` (e.g. for TDAGs)
            Kw kw = reverseKwOrder ? dbSize - i + 1 : i;
            db.push_back(DbEntry {Doc(i, kw, Op::INS), Range<Kw> {kw, kw}});
            // delete the document with keyword 4
            if (kw == 4) {
                db.push_back(DbEntry {Doc(i, kw, Op::DEL), Range<Kw> {kw, kw}});
            }
        }
    } else {
        for (long i = 0; i < dbSize; i++) {
            Kw kw = reverseKwOrder ? dbSize - i + 2 : i;
            db.push_back(DbEntry {Doc(i, kw, Op::INS), Range<Kw> {kw, kw}});
        }
    }

    return db;
}


// experiment for debugging with fixed query and printed results
void expDebug(ISse<>& sse, long dbSize, Range<Kw> query) {
    if (dbSize == 0) {
        return;
    }
    // adjust params at will
    Db<> db = createUniformDb(dbSize, true, true);

    // setup
    sse.setup(KEY_LEN, db);

    // search
    std::vector<Doc> results = sse.search(query);
    std::cout << "Results (id,kw,op):" << std::endl;
    for (Doc result : results) {
        std::cout << result << " with keyword " << result.getKw() << std::endl;
    }
    std::cout << std::endl;

    sse.clear();
}


void exp1(ISse<>& sse, long dbSize) {
    if (dbSize == 0) {
        return;
    }
    Db<> db = createUniformDb(dbSize, false, false);

    // setup
    auto setupStart = std::chrono::high_resolution_clock::now();
    sse.setup(KEY_LEN, db);
    auto setupEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> setupElapsed = setupEnd - setupStart;
    std::cout << "Setup time: " << setupElapsed.count() * 1000 << " ms" << std::endl;
    std::cout << std::endl;

    // search
    for (long i = 0; i <= log2(dbSize); i++) {
        Range<Kw> query {0, (long)pow(2, i) - 1};
        auto searchStartTime = std::chrono::high_resolution_clock::now();
        sse.search(query);
        auto searchEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> searchElapsed = searchEndTime - searchStartTime;
        std::cout << "Search time (size 2^" << log2(query.size()) << "): " << searchElapsed.count() * 1000
                  << " ms" << std::endl;
    }
    std::cout << std::endl;

    sse.clear();
}


void exp2(ISse<>& sse, long maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    Range<Kw> query {0, 3};

    for (long i = 2; i <= log2(maxDbSize); i++) {
        long dbSize = pow(2, i);
        Db<> db = createUniformDb(dbSize, false, false);

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

    sse.clear();
}


void exp3(ISse<>& sse, long maxDbSize) {
    if (maxDbSize == 0) {
        return;
    }
    for (long i = 2; i <= log2(maxDbSize); i++) {
        long dbSize = pow(2, i);
        // two unique keywords, with all but one being 0 and the other being the max
        // thus all but one doc will be returned as false positives on a [1, n - 1] query (if the root node is the SRC)
        Db<> db;
        long kw1 = 0;
        long kw2 = dbSize - 1;
        for (long i = 0; i < dbSize - 1; i++) {
            db.push_back(DbEntry {Doc(i, kw1, Op::INS), Range<Kw> {kw1, kw1}});
        }
        db.push_back(DbEntry {Doc(dbSize - 1, kw2, Op::INS), Range<Kw> {kw2, kw2}});

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

    sse.clear();
}


int main() {
    long maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    long maxDbSize = pow(2, maxDbSizeExp);

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

    std::cout << "----------------------------- Debugging Experiment -----------------------------" << std::endl;
    std::cout << "DB size  : 2^" << maxDbSizeExp                                                    << std::endl;
    std::cout << "Query    : "   << query                                                           << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    expDebug(piBas, maxDbSize, query);

    std::cout << "------------ PiBasResHiding ------------" << std::endl;
    std::cout << std::endl;
    expDebug(piBasResHiding, maxDbSize, query);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrc, maxDbSize, query);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    expDebug(logSrcI, maxDbSize, query);

    std::cout << "--------- SDa[PiBasResHiding] ----------" << std::endl;
    std::cout << std::endl;
    expDebug(sdaPiBas, maxDbSize, query);

    std::cout << "----- SDa[Log-SRC[PiBasResHiding]] -----" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrc, maxDbSize, query);

    std::cout << "---- SDa[Log-SRC-i[PiBasResHiding]] ----" << std::endl;
    std::cout << std::endl;
    expDebug(sdaLogSrcI, maxDbSize, query);

    /////////////////////////////// experiment 1 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 1 ---------------------------------" << std::endl;
    std::cout << "DB size  : 2^" << maxDbSizeExp                                                    << std::endl;
    std::cout << "Query    : varied"                                                                << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp1(piBas, maxDbSize);

    std::cout << "------------ PiBasResHiding ------------" << std::endl;
    std::cout << std::endl;
    exp1(piBasResHiding, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp1(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp1(logSrcI, maxDbSize);

    std::cout << "--------- SDa[PiBasResHiding] ----------" << std::endl;
    std::cout << std::endl;
    exp1(sdaPiBas, maxDbSize);

    std::cout << "----- SDa[Log-SRC[PiBasResHiding]] -----" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrc, maxDbSize);

    std::cout << "---- SDa[Log-SRC-i[PiBasResHiding]] ----" << std::endl;
    std::cout << std::endl;
    exp1(sdaLogSrcI, maxDbSize);

    /////////////////////////////// experiment 2 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 2 ---------------------------------" << std::endl;
    std::cout << "DB size  : varied, up to 2^" << maxDbSizeExp                                      << std::endl;
    std::cout << "Query    : 0-3"                                                                   << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "---------------- PiBas -----------------" << std::endl;
    std::cout << std::endl;
    exp2(piBas, maxDbSize);

    std::cout << "------------ PiBasResHiding ------------" << std::endl;
    std::cout << std::endl;
    exp2(piBasResHiding, maxDbSize);

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp2(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp2(logSrcI, maxDbSize);

    std::cout << "--------- SDa[PiBasResHiding] ----------" << std::endl;
    std::cout << std::endl;
    exp2(sdaPiBas, maxDbSize);

    std::cout << "----- SDa[Log-SRC[PiBasResHiding]] -----" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrc, maxDbSize);

    std::cout << "---- SDa[Log-SRC-i[PiBasResHiding]] ----" << std::endl;
    std::cout << std::endl;
    exp2(sdaLogSrcI, maxDbSize);
    
    /////////////////////////////// experiment 3 ///////////////////////////////

    std::cout << "--------------------------------- Experiment 3 ---------------------------------" << std::endl;
    std::cout << "DB size  : 2^" << maxDbSizeExp                                                    << std::endl;
    std::cout << "Query    : high false positives"                                                  << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "------------ Log-SRC[PiBas] ------------" << std::endl;
    std::cout << std::endl;
    exp3(logSrc, maxDbSize);

    std::cout << "----------- Log-SRC-i[PiBas] -----------" << std::endl;
    std::cout << std::endl;
    exp3(logSrcI, maxDbSize);
}
