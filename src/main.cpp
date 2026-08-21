#include <cmath>
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

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"


int main() {
    bigint maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    const bigint maxDbSize = std::pow(2, maxDbSizeExp);
    std::cout << std::endl << std::endl;

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
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<NLogN<>>> sdaNLogN               = app::createDsse<Sda<NLogN<>>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<PiBas>>> sdaLogSrcPiBas   = app::createDsse<Sda<LogSrc<PiBas>>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<NLogN>>> sdaLogSrcNLogN   = app::createDsse<Sda<LogSrc<NLogN>>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<PiBas>>> sdaLogSrcIPiBas = app::createDsse<Sda<LogSrcI<PiBas>>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<NLogN>>> sdaLogSrcINLogN = app::createDsse<Sda<LogSrcI<NLogN>>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcIStar>> sdaLogSrcIStar     = app::createDsse<Sda<LogSrcIStar>>(
        config::SHOULD_BENCHMARK,
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );

    //--------------------------------------------------------------------------
    // debugging experiment

    app::experiments::Debugging debugging(maxDbSizeExp);
    debugging.printHeader();

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    debugging.run(piBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    debugging.run(nLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    debugging.run(logSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    debugging.run(logSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    debugging.run(logSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    debugging.run(logSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    debugging.run(logSrcIStar.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    debugging.run(sdaPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    debugging.run(sdaNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    debugging.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    debugging.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    debugging.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    debugging.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    debugging.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);

    // free memory ASAP
    debugging.clearDb();

    //--------------------------------------------------------------------------
    // all vs. DB size experiment

    app::experiments::AllVsDbSize allVsDbSize(maxDbSizeExp);
    allVsDbSize.printHeader();

    /*
    std::cout << "================ PiBas =================" << std::endl << std::endl;
    allVsDbSize.run(piBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    allVsDbSize.run(nLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    allVsDbSize.run(logSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    allVsDbSize.run(logSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    allVsDbSize.run(logSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    allVsDbSize.run(logSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    allVsDbSize.run(logSrcIStar.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    allVsDbSize.run(sdaPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    allVsDbSize.run(sdaNLogN.get(), config::SHOULD_BENCHMARK);
    */

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    allVsDbSize.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    allVsDbSize.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    allVsDbSize.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    allVsDbSize.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    allVsDbSize.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);

    /*
    //--------------------------------------------------------------------------
    // search vs. result size experiment

    app::experiments::SearchVsResultSize searchVsResultSize(maxDbSizeExp);
    searchVsResultSize.printHeader();

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    searchVsResultSize.run(piBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    searchVsResultSize.run(nLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    searchVsResultSize.run(logSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    searchVsResultSize.run(logSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    searchVsResultSize.run(logSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    searchVsResultSize.run(logSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    searchVsResultSize.run(logSrcIStar.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    searchVsResultSize.run(sdaPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    searchVsResultSize.run(sdaNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    searchVsResultSize.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    searchVsResultSize.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    searchVsResultSize.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    searchVsResultSize.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    searchVsResultSize.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);

    //--------------------------------------------------------------------------
    // search vs. range size experiment

    app::experiments::SearchVsRangeSize searchVsRangeSize(maxDbSizeExp);
    searchVsRangeSize.printHeader();

    std::cout << "================ PiBas =================" << std::endl << std::endl;
    searchVsRangeSize.run(piBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "================ NLogN =================" << std::endl << std::endl;
    searchVsRangeSize.run(nLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    searchVsRangeSize.run(logSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    searchVsRangeSize.run(logSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    searchVsRangeSize.run(logSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    searchVsRangeSize.run(logSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    searchVsRangeSize.run(logSrcIStar.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
    searchVsRangeSize.run(sdaPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
    searchVsRangeSize.run(sdaNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    searchVsRangeSize.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    searchVsRangeSize.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    searchVsRangeSize.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    searchVsRangeSize.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    searchVsRangeSize.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);
    
    //--------------------------------------------------------------------------
    // search vs. false positives experiment

    app::experiments::SearchVsFalsePos searchVsFalsePos(maxDbSizeExp);
    searchVsFalsePos.printHeader();

    std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
    searchVsFalsePos.run(logSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
    searchVsFalsePos.run(logSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
    searchVsFalsePos.run(logSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
    searchVsFalsePos.run(logSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
    searchVsFalsePos.run(logSrcIStar.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
    searchVsFalsePos.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
    searchVsFalsePos.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

    std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
    searchVsFalsePos.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
    searchVsFalsePos.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

    std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
    searchVsFalsePos.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);

    //--------------------------------------------------------------------------
    // updates experiment

    if (config::SHOULD_BENCHMARK_UPDTS) {
        app::experiments::UpdateVsDbSize updateVsDbSize(maxDbSizeExp);
        updateVsDbSize.printHeader();

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        updateVsDbSize.run(sdaPiBas.get(), config::SHOULD_BENCHMARK);

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        updateVsDbSize.run(sdaNLogN.get(), config::SHOULD_BENCHMARK);

        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcPiBas.get(), config::SHOULD_BENCHMARK);

        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcNLogN.get(), config::SHOULD_BENCHMARK);

        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcIPiBas.get(), config::SHOULD_BENCHMARK);

        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcINLogN.get(), config::SHOULD_BENCHMARK);

        // (note that this has horrendous storage and performance as randomized keywords lets
        // individual SDa subindexes have HUGE TDAG 1s. for the same reason this is not secure)
        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcIStar.get(), config::SHOULD_BENCHMARK);

        // free memory ASAP
        updateVsDbSize.clearDb();
    }
    */
}
