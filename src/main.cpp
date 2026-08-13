#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>

#include "config.h"

#include "app/db_factory.h"
#include "app/sse_factory.h"
#include "app/experiments/experiments.h"

#include "schemes/log_src/log_src.h"
#include "schemes/log_src_i/log_src_i.h"
#include "schemes/log_src_i_star/log_src_i_star.h"
#include "schemes/n_log_n/n_log_n.h"
#include "schemes/pi_bas/pi_bas.h"
#include "schemes/sda/sda.h"

#include "utils/range.h"
#include "utils/types.h"


int main() {
    int64_t maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    const int64_t maxDbSize = std::pow(2, maxDbSizeExp);
    std::cout << std::endl;

    std::unique_ptr<PiBas<>> piBas               = app::createSse<PiBas<>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<NLogN<>> nLogN               = app::createSse<NLogN<>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrc<PiBas>> logSrcPiBas   = app::createSse<LogSrc<PiBas>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrc<NLogN>> logSrcNLogN   = app::createSse<LogSrc<NLogN>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcI<PiBas>> logSrcIPiBas = app::createSse<LogSrcI<PiBas>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcI<NLogN>> logSrcINLogN = app::createSse<LogSrcI<NLogN>>(
        config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcIStar> logSrcIStar     = app::createSse<LogSrcIStar>(
        config::SHOULD_BENCHMARK
    );

    std::unique_ptr<Sda<PiBas<>>> sdaPiBas               = app::createDsse<Sda<PiBas<>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<NLogN<>>> sdaNLogN               = app::createDsse<Sda<NLogN<>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<PiBas>>> sdaLogSrcPiBas   = app::createDsse<Sda<LogSrc<PiBas>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<NLogN>>> sdaLogSrcNLogN   = app::createDsse<Sda<LogSrc<NLogN>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<PiBas>>> sdaLogSrcIPiBas = app::createDsse<Sda<LogSrcI<PiBas>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<NLogN>>> sdaLogSrcINLogN = app::createDsse<Sda<LogSrcI<NLogN>>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcIStar>> sdaLogSrcIStar     = app::createDsse<Sda<LogSrcIStar>>(
        config::SHOULD_BENCHMARK,
        config::DSSE_USE_SHORTCUT_SETUP, config::DSSE_SHOULD_BENCHMARK_UPDTS
    );

    //--------------------------------------------------------------------------
    // debugging experiment

    // (this is out here so all the schemes get the same DB (if it was randomized))
    Range<Kw> query {3, 5};
    Db<> db = app::createDb(maxDbSize, true, true); // adjust params at will

    std::cout << std::endl;
    std::cout << "============================= Debugging Experiment ============================="
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Fixed query " << query << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::debuggingExp(piBas.get(), db, query);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::debuggingExp(nLogN.get(), db, query);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::debuggingExp(logSrcPiBas.get(), db, query);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::debuggingExp(logSrcNLogN.get(), db, query);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::debuggingExp(logSrcIPiBas.get(), db, query);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::debuggingExp(logSrcINLogN.get(), db, query);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::debuggingExp(logSrcIStar.get(), db, query);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaPiBas.get(), db, query);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaNLogN.get(), db, query);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaLogSrcPiBas.get(), db, query);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaLogSrcNLogN.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaLogSrcIPiBas.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaLogSrcINLogN.get(), db, query);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::debuggingExp(sdaLogSrcIStar.get(), db, query);

    /*
    //--------------------------------------------------------------------------
    // DB sizes experiment

    std::cout << std::endl;
    std::cout << "============================= DB Sizes Experiment =============================="
              << std::endl;
    std::cout << "Varied DB size up to 2^" << maxDbSizeExp << std::endl;
    std::cout << "Fixed query 0-3" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::dbSizesExp(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::dbSizesExp(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::dbSizesExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::dbSizesExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::dbSizesExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::dbSizesExp(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::dbSizesExp(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::dbSizesExp(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // search sizes experiment

    std::cout << std::endl;
    std::cout << "=========================== Search Sizes Experiment ============================"
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Varied query range size" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::searchSizesExp(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::searchSizesExp(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::searchSizesExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::searchSizesExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::searchSizesExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::searchSizesExp(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::searchSizesExp(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::searchSizesExp(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // result sizes experiment

    std::cout << std::endl;
    std::cout << "=========================== Result Sizes Experiment ============================"
              << std::endl;
    std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
    std::cout << "Varied query result size" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::resultSizesExp(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::resultSizesExp(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::resultSizesExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::resultSizesExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::resultSizesExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::resultSizesExp(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::resultSizesExp(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::resultSizesExp(sdaLogSrcIStar.get(), maxDbSize);
    
    //--------------------------------------------------------------------------
    // false positives experiment

    std::cout << std::endl;
    std::cout << "========================== False Positives Experiment =========================="
              << std::endl;
    std::cout << "Varied DB size up to 2^" << maxDbSizeExp << std::endl;
    std::cout << "High false positives query" << std::endl;
    std::cout << "================================================================================"
              << std::endl;
    std::cout << std::endl << std::endl;

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::falsePosExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::falsePosExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::falsePosExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::falsePosExp(logSrcINLogN.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // updates experiment

    if (config::DSSE_SHOULD_BENCHMARK_UPDTS) {
        std::cout << std::endl;
        std::cout << "============================== Updates Experiment =============================="
                  << std::endl;
        std::cout << "Fixed DB size 2^" << maxDbSizeExp << std::endl;
        std::cout << "One update at a time" << std::endl;
        std::cout << "================================================================================"
                  << std::endl;
        std::cout << std::endl << std::endl;

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        app::experiments::updatesExp(sdaPiBas.get(), maxDbSize);

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        app::experiments::updatesExp(sdaNLogN.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        app::experiments::updatesExp(sdaLogSrcPiBas.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        app::experiments::updatesExp(sdaLogSrcNLogN.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        app::experiments::updatesExp(sdaLogSrcIPiBas.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        app::experiments::updatesExp(sdaLogSrcINLogN.get(), maxDbSize);

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        app::experiments::updatesExp(sdaLogSrcIStar.get(), maxDbSize);
    }
    */
}
