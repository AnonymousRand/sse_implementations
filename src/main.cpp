#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>

#include "config.h"
#include "experiments.cpp"

#include "core/sse_factory.h"

#include "schemes/log_src.h"
#include "schemes/log_src_i.h"
#include "schemes/log_src_i_star.h"
#include "schemes/n_log_n.h"
#include "schemes/pi_bas.h"
#include "schemes/sda.h"

#include "utils/range.h"
#include "utils/sse_utils.h"


int main() {
    int64_t maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    const int64_t maxDbSize = std::pow(2, maxDbSizeExp);
    std::cout << std::endl;

    std::unique_ptr<PiBas<>> piBas               = createSse<PiBas<>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<NLogN<>> nLogN               = createSse<NLogN<>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrc<PiBas>> logSrcPiBas   = createSse<LogSrc<PiBas>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrc<NLogN>> logSrcNLogN   = createSse<LogSrc<NLogN>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcI<PiBas>> logSrcIPiBas = createSse<LogSrcI<PiBas>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcI<NLogN>> logSrcINLogN = createSse<LogSrcI<NLogN>>(
        Config::SHOULD_BENCHMARK
    );
    std::unique_ptr<LogSrcIStar> logSrcIStar     = createSse<LogSrcIStar>(
        Config::SHOULD_BENCHMARK
    );

    std::unique_ptr<Sda<PiBas<>>> sdaPiBas               = createDsse<Sda<PiBas<>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<NLogN<>>> sdaNLogN               = createDsse<Sda<NLogN<>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<PiBas>>> sdaLogSrcPiBas   = createDsse<Sda<LogSrc<PiBas>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<NLogN>>> sdaLogSrcNLogN   = createDsse<Sda<LogSrc<NLogN>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<PiBas>>> sdaLogSrcIPiBas = createDsse<Sda<LogSrcI<PiBas>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<NLogN>>> sdaLogSrcINLogN = createDsse<Sda<LogSrcI<NLogN>>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcIStar>> sdaLogSrcIStar     = createDsse<Sda<LogSrcIStar>>(
        Config::SHOULD_BENCHMARK, Config::DSSE_USE_SHORTCUT_SETUP, Config::DSSE_SHOULD_BENCHMARK_UPDTS
    );

    //--------------------------------------------------------------------------
    // debugging experiment

    // (this is out here so all the schemes get the same DB (if it was randomized))
    Range<Kw> query {3, 5};
    Db<> db = Experiments::createDb(maxDbSize, true, true); // adjust params at will

    std::cout << "============================= Debugging Experiment =============================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : "   << query                                                             << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(piBas.get(), db, query);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(nLogN.get(), db, query);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(logSrcPiBas.get(), db, query);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(logSrcNLogN.get(), db, query);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(logSrcIPiBas.get(), db, query);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(logSrcINLogN.get(), db, query);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(logSrcIStar.get(), db, query);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaPiBas.get(), db, query);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaNLogN.get(), db, query);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaLogSrcPiBas.get(), db, query);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaLogSrcNLogN.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaLogSrcIPiBas.get(), db, query);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaLogSrcINLogN.get(), db, query);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    Experiments::debuggingExp(sdaLogSrcIStar.get(), db, query);

    //--------------------------------------------------------------------------
    // search sizes experiment

    std::cout << "=========================== Search Sizes Experiment ============================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "Query  : varied"                                                                  << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    Experiments::searchSizesExp(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // setup sizes experiment

    std::cout << "============================ Setup Sizes Experiment ============================" << std::endl;
    std::cout << "DB size: varied, up to 2^" << maxDbSizeExp                                        << std::endl;
    std::cout << "Query  : 0-3"                                                                     << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "================ PiBas =================" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
    std::cout << std::endl;
    Experiments::setupSizesExp(sdaLogSrcIStar.get(), maxDbSize);
    
    //--------------------------------------------------------------------------
    // false positives experiment

    std::cout << "========================== False Positives Experiment ==========================" << std::endl;
    std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
    std::cout << "High false positives query"                                                       << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl;
    std::cout << std::endl;
    Experiments::falsePosExp(logSrcPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::falsePosExp(logSrcIPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl;
    std::cout << std::endl;
    Experiments::falsePosExp(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl;
    std::cout << std::endl;
    Experiments::falsePosExp(logSrcINLogN.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // updates experiment

    if (Config::DSSE_SHOULD_BENCHMARK_UPDTS) {
        std::cout << "============================== Updates Experiment ==============================" << std::endl;
        std::cout << "DB size: 2^" << maxDbSizeExp                                                      << std::endl;
        std::cout << "Updates one at a time"                                                            << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::cout << std::endl;

        std::cout << "============== SDa[PiBas] ==============" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaPiBas.get(), maxDbSize);

        std::cout << "============== SDa[NLogN] ==============" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaNLogN.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaLogSrcPiBas.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaLogSrcNLogN.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaLogSrcIPiBas.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaLogSrcINLogN.get(), maxDbSize);

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl;
        std::cout << std::endl;
        Experiments::updatesExp(sdaLogSrcIStar.get(), maxDbSize);
    }
}
