#include <algorithm>
#include <cmath>
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

#include "utils/types/basic_types.h"
#include "utils/types/db/db.h"
#include "utils/types/range.h"


int main() {
    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    std::cout << std::endl << std::endl;
    // smaller sizes, e.g. for Log-SRC[NLogN]-based things where storage is log^2
    const int maxDbSizeExpSmall = maxDbSizeExp > 15 ? std::max(maxDbSizeExp - 5, 15) : maxDbSizeExp;

    std::unique_ptr<PiBas<>>        piBas         = app::createSse<PiBas<>>();
    std::unique_ptr<NLogN<>>        nLogN         = app::createSse<NLogN<>>();
    std::unique_ptr<LogSrc<PiBas>>  logSrcPiBas   = app::createSse<LogSrc<PiBas>>();
    std::unique_ptr<LogSrc<NLogN>>  logSrcNLogN   = app::createSse<LogSrc<NLogN>>();
    std::unique_ptr<LogSrcI<PiBas>> logSrcIPiBas  = app::createSse<LogSrcI<PiBas>>();
    std::unique_ptr<LogSrcI<NLogN>> logSrcINLogN  = app::createSse<LogSrcI<NLogN>>();
    std::unique_ptr<LogSrcIStar>    logSrcIStar   = app::createSse<LogSrcIStar>();

    std::unique_ptr<Sda<PiBas<>>>        sdaPiBas        = app::createDsse<Sda<PiBas<>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<NLogN<>>>        sdaNLogN        = app::createDsse<Sda<NLogN<>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<PiBas>>>  sdaLogSrcPiBas  = app::createDsse<Sda<LogSrc<PiBas>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrc<NLogN>>>  sdaLogSrcNLogN  = app::createDsse<Sda<LogSrc<NLogN>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<PiBas>>> sdaLogSrcIPiBas = app::createDsse<Sda<LogSrcI<PiBas>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcI<NLogN>>> sdaLogSrcINLogN = app::createDsse<Sda<LogSrcI<NLogN>>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );
    std::unique_ptr<Sda<LogSrcIStar>>    sdaLogSrcIStar  = app::createDsse<Sda<LogSrcIStar>>(
        config::USE_SHORTCUT_DSSE_SETUP, config::SHOULD_BENCHMARK_UPDTS
    );

    //--------------------------------------------------------------------------
    // debugging experiment

    {
        // DB, query declared here so they don't change between `run()` calls with different schemes
        // this also means that this experiment must use the same DB and hence DB size everywhere
        const int dbSizeExp = maxDbSizeExpSmall;
        Db<> db;
        app::createDb(db, std::pow(2, dbSizeExp), true, true);
        const Range<Kw> query {3, 5};
        app::experiments::Debugging debugging(db, query);
        debugging.printHeader();

        std::cout << "================ PiBas =================" << std::endl << std::endl;
        debugging.run(piBas.get());

        std::cout << "================ NLogN =================" << std::endl << std::endl;
        debugging.run(nLogN.get());

        // slow setup
        std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
        debugging.run(logSrcPiBas.get());

        // huge storage
        std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
        debugging.run(logSrcNLogN.get());

        // slow setup
        std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
        debugging.run(logSrcIPiBas.get());

        // huge storage
        std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
        debugging.run(logSrcINLogN.get());

        std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
        debugging.run(logSrcIStar.get());

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        debugging.run(sdaPiBas.get());

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        debugging.run(sdaNLogN.get());

        // slow setup
        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        debugging.run(sdaLogSrcPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        debugging.run(sdaLogSrcNLogN.get());

        // slow setup
        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        debugging.run(sdaLogSrcIPiBas.get());

        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        debugging.run(sdaLogSrcINLogN.get());

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        debugging.run(sdaLogSrcIStar.get());

        // free memory ASAP
        db.clear();
    }

    //--------------------------------------------------------------------------
    // all vs. DB size experiment

    {
        const bigint targetResSize = 100;
        const int maxDbSizeExpSmallish =
            maxDbSizeExp > 15 ? std::max(maxDbSizeExp - 3, 15) : maxDbSizeExp;
        app::experiments::AllVsDbSize allVsDbSize(maxDbSizeExp, targetResSize);
        app::experiments::AllVsDbSize allVsDbSizeSmallish(maxDbSizeExpSmallish, targetResSize);
        app::experiments::AllVsDbSize allVsDbSizeSmall(maxDbSizeExpSmall, targetResSize);
        allVsDbSize.printHeader();

        std::cout << "================ PiBas =================" << std::endl << std::endl;
        allVsDbSize.run(piBas.get());

        std::cout << "================ NLogN =================" << std::endl << std::endl;
        allVsDbSize.run(nLogN.get());

        // slow setup
        std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
        allVsDbSizeSmallish.run(logSrcPiBas.get());

        // huge storage
        std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
        allVsDbSizeSmall.run(logSrcNLogN.get());

        // slow setup
        std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
        allVsDbSizeSmallish.run(logSrcIPiBas.get());

        // huge storage
        std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
        allVsDbSizeSmall.run(logSrcINLogN.get());

        std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
        allVsDbSize.run(logSrcIStar.get());

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        allVsDbSize.run(sdaPiBas.get());

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        allVsDbSize.run(sdaNLogN.get());

        // slow setup
        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        allVsDbSizeSmallish.run(sdaLogSrcPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        allVsDbSizeSmall.run(sdaLogSrcNLogN.get());

        // slow setup
        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        allVsDbSizeSmallish.run(sdaLogSrcIPiBas.get());
        
        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        allVsDbSizeSmall.run(sdaLogSrcINLogN.get());

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        allVsDbSize.run(sdaLogSrcIStar.get());
    }

    //--------------------------------------------------------------------------
    // search vs. result size experiment

    {
        // this experiment must use the same DB size everywhere
        const int dbSizeExp = maxDbSizeExpSmall;
        const int maxResSizeExp = 22;
        const int maxResSizeExpSmall = 18;
        app::experiments::SearchVsResSize searchVsResSize(dbSizeExp, maxResSizeExp);
        app::experiments::SearchVsResSize searchVsResSizeSmall(dbSizeExp, maxResSizeExpSmall);
        searchVsResSize.printHeader();

        // slow searches
        std::cout << "================ PiBas =================" << std::endl << std::endl;
        searchVsResSizeSmall.run(piBas.get());

        // slow searches
        std::cout << "================ NLogN =================" << std::endl << std::endl;
        searchVsResSizeSmall.run(nLogN.get());

        // slow setup
        std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
        searchVsResSize.run(logSrcPiBas.get());

        // huge storage
        std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
        searchVsResSize.run(logSrcNLogN.get());

        // slow setup
        std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
        searchVsResSize.run(logSrcIPiBas.get());

        // huge storage
        std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
        searchVsResSize.run(logSrcINLogN.get());

        std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
        searchVsResSize.run(logSrcIStar.get());

        // slow searches
        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        searchVsResSizeSmall.run(sdaPiBas.get());

        // slow searches
        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        searchVsResSizeSmall.run(sdaNLogN.get());

        // slow setup
        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        searchVsResSize.run(sdaLogSrcPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        searchVsResSize.run(sdaLogSrcNLogN.get());

        // slow setup
        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        searchVsResSize.run(sdaLogSrcIPiBas.get());

        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        searchVsResSize.run(sdaLogSrcINLogN.get());

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        searchVsResSize.run(sdaLogSrcIStar.get());
    }

    //--------------------------------------------------------------------------
    // search vs. range size experiment

    {
        // this experiment must use the same DB size everywhere
        const int dbSizeExp = maxDbSizeExpSmall;
        const int maxRangeSizeExp = 22;
        const int maxRangeSizeExpSmall = 18;
        app::experiments::SearchVsRangeSize searchVsRangeSize(dbSizeExp, maxRangeSizeExp);
        app::experiments::SearchVsRangeSize searchVsRangeSizeSmall(dbSizeExp, maxRangeSizeExpSmall);
        searchVsRangeSize.printHeader();

        // slow searches
        std::cout << "================ PiBas =================" << std::endl << std::endl;
        searchVsRangeSizeSmall.run(piBas.get());

        // slow searches
        std::cout << "================ NLogN =================" << std::endl << std::endl;
        searchVsRangeSizeSmall.run(nLogN.get());

        // slow setup
        std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
        searchVsRangeSize.run(logSrcPiBas.get());

        // huge storage
        std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
        searchVsRangeSize.run(logSrcNLogN.get());

        // slow setup
        std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
        searchVsRangeSize.run(logSrcIPiBas.get());

        // huge storage
        std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
        searchVsRangeSize.run(logSrcINLogN.get());

        std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
        searchVsRangeSize.run(logSrcIStar.get());

        // slow searches
        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        searchVsRangeSizeSmall.run(sdaPiBas.get());

        // slow searches
        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        searchVsRangeSizeSmall.run(sdaNLogN.get());

        // slow setup
        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        searchVsRangeSize.run(sdaLogSrcPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        searchVsRangeSize.run(sdaLogSrcNLogN.get());

        // slow setup
        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        searchVsRangeSize.run(sdaLogSrcIPiBas.get());

        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        searchVsRangeSize.run(sdaLogSrcINLogN.get());

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        searchVsRangeSize.run(sdaLogSrcIStar.get());
    }
    
    //--------------------------------------------------------------------------
    // search vs. false positives experiment

    {
        // this experiment must use the same DB size everywhere
        const int dbSizeExp = maxDbSizeExpSmall;
        app::experiments::SearchVsFalsePos searchVsFalsePos(dbSizeExp);
        searchVsFalsePos.printHeader();

        // slow setup
        std::cout << "============ Log-SRC[PiBas] ============" << std::endl << std::endl;
        searchVsFalsePos.run(logSrcPiBas.get());

        // slow setup
        std::cout << "=========== Log-SRC-i[PiBas] ===========" << std::endl << std::endl;
        searchVsFalsePos.run(logSrcIPiBas.get());

        // huge storage
        std::cout << "============ Log-SRC[NLogN] ============" << std::endl << std::endl;
        searchVsFalsePos.run(logSrcNLogN.get());

        // huge storage
        std::cout << "=========== Log-SRC-i[NLogN] ===========" << std::endl << std::endl;
        searchVsFalsePos.run(logSrcINLogN.get());

        std::cout << "============== Log-SRC-i* ==============" << std::endl << std::endl;
        searchVsFalsePos.run(logSrcIStar.get());

        // slow setup
        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        searchVsFalsePos.run(sdaLogSrcPiBas.get());

        // slow setup
        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        searchVsFalsePos.run(sdaLogSrcIPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        searchVsFalsePos.run(sdaLogSrcNLogN.get());

        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        searchVsFalsePos.run(sdaLogSrcINLogN.get());

        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        searchVsFalsePos.run(sdaLogSrcIStar.get());
    }

    //--------------------------------------------------------------------------
    // update vs. DB size experiment

    if (config::SHOULD_BENCHMARK_UPDTS) {
        // set a bound to prevent this experiment from taking too long and outputting too much text
        const int dbSizeExp = std::min(maxDbSizeExp, 16);
        const int dbSizeExpSmall = std::min(maxDbSizeExpSmall, 12);
        app::experiments::UpdateVsDbSize updateVsDbSize(dbSizeExp);
        app::experiments::UpdateVsDbSize updateVsDbSizeSmall(dbSizeExpSmall);
        updateVsDbSize.printHeader();

        std::cout << "============== SDa[PiBas] ==============" << std::endl << std::endl;
        updateVsDbSize.run(sdaPiBas.get());

        std::cout << "============== SDa[NLogN] ==============" << std::endl << std::endl;
        updateVsDbSize.run(sdaNLogN.get());

        std::cout << "========= SDa[Log-SRC[PiBas]] ==========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcPiBas.get());

        // huge storage
        std::cout << "========= SDa[Log-SRC[NLogN]] ==========" << std::endl << std::endl;
        updateVsDbSizeSmall.run(sdaLogSrcNLogN.get());

        std::cout << "======== SDa[Log-SRC-i[PiBas]] =========" << std::endl << std::endl;
        updateVsDbSize.run(sdaLogSrcIPiBas.get());

        // huge storage
        std::cout << "======== SDa[Log-SRC-i[NLogN]] =========" << std::endl << std::endl;
        updateVsDbSizeSmall.run(sdaLogSrcINLogN.get());

        // slow updates
        // (as randomized keywords with Log-SRC-i*'s TDAG 1 contiguousness padding lets
        // individual SDa subindexes have HUGE TDAG 1s. for the same reason this is not secure)
        std::cout << "=========== SDa[Log-SRC-i*] ============" << std::endl << std::endl;
        updateVsDbSizeSmall.run(sdaLogSrcIStar.get());
    }
}
