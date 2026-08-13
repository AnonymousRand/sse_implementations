#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>

#include "config.h"

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

    // declare these out here so that they don't change (in particular, randomized DBs)
    // during any one program execution, between different SSE schemes/calls to `run()`
    // adjust at will here!
    Db<> debuggingDb = app::createDb(maxDbSize, true, true);
    Range<Kw> debuggingQuery {3, 5};
    app::experiments::debugging::init(&debuggingDb, debuggingQuery);

    app::experiments::debugging::printHeader(maxDbSizeExp);

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::debugging::run(piBas.get());

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::debugging::run(nLogN.get());

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::debugging::run(logSrcPiBas.get());

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::debugging::run(logSrcNLogN.get());

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::debugging::run(logSrcIPiBas.get());

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::debugging::run(logSrcINLogN.get());

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::debugging::run(logSrcIStar.get());

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::debugging::run(sdaPiBas.get());

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::debugging::run(sdaNLogN.get());

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::debugging::run(sdaLogSrcPiBas.get());

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::debugging::run(sdaLogSrcNLogN.get());

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::debugging::run(sdaLogSrcIPiBas.get());

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::debugging::run(sdaLogSrcINLogN.get());

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::debugging::run(sdaLogSrcIStar.get());

    // to save memory
    debuggingDb.clear();

    //--------------------------------------------------------------------------
    // DB sizes experiment

    app::experiments::db_sizes::printHeader(maxDbSizeExp);

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::db_sizes::run(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::db_sizes::run(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::db_sizes::run(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::db_sizes::run(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::db_sizes::run(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::db_sizes::run(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::db_sizes::run(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::db_sizes::run(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // search sizes experiment

    app::experiments::search_sizes::printHeader(maxDbSizeExp);

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::search_sizes::run(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::search_sizes::run(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::search_sizes::run(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::search_sizes::run(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::search_sizes::run(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::search_sizes::run(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::search_sizes::run(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::search_sizes::run(sdaLogSrcIStar.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // result sizes experiment

    app::experiments::result_sizes::printHeader(maxDbSizeExp);

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    app::experiments::result_sizes::run(piBas.get(), maxDbSize);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    app::experiments::result_sizes::run(nLogN.get(), maxDbSize);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::result_sizes::run(logSrcPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::result_sizes::run(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::result_sizes::run(logSrcIPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::result_sizes::run(logSrcINLogN.get(), maxDbSize);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    app::experiments::result_sizes::run(logSrcIStar.get(), maxDbSize);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaPiBas.get(), maxDbSize);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaNLogN.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaLogSrcPiBas.get(), maxDbSize);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaLogSrcNLogN.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaLogSrcIPiBas.get(), maxDbSize);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaLogSrcINLogN.get(), maxDbSize);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    app::experiments::result_sizes::run(sdaLogSrcIStar.get(), maxDbSize);
    
    //--------------------------------------------------------------------------
    // false positives experiment

    app::experiments::false_pos::printHeader(maxDbSizeExp);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    app::experiments::false_pos::run(logSrcPiBas.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    app::experiments::false_pos::run(logSrcIPiBas.get(), maxDbSize);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    app::experiments::false_pos::run(logSrcNLogN.get(), maxDbSize);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    app::experiments::false_pos::run(logSrcINLogN.get(), maxDbSize);

    //--------------------------------------------------------------------------
    // updates experiment

    if (config::DSSE_SHOULD_BENCHMARK_UPDTS) {
        app::experiments::updates::printHeader(maxDbSizeExp);

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        app::experiments::updates::run(sdaPiBas.get(), maxDbSize);

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        app::experiments::updates::run(sdaNLogN.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        app::experiments::updates::run(sdaLogSrcPiBas.get(), maxDbSize);

        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        app::experiments::updates::run(sdaLogSrcNLogN.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        app::experiments::updates::run(sdaLogSrcIPiBas.get(), maxDbSize);

        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        app::experiments::updates::run(sdaLogSrcINLogN.get(), maxDbSize);

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        app::experiments::updates::run(sdaLogSrcIStar.get(), maxDbSize);
    }
}
