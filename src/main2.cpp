#include <algorithm>
#include <cmath>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"

Db<> createDb(int dbSize) {
    Db<> db;
    for (int i = 0; i < dbSize; i++) {
        Kw kw = i;
        db.push_back(std::pair {Id(i), KwRange {kw, kw}});
    }
    std::sort(db.begin(), db.end());
    return db;
}

int main() {
    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    int maxDbSize = pow(2, maxDbSizeExp);
    Db<> db = createDb(maxDbSize);

    PiBas piBas;
    LogSrci<> logSrci(piBas);
    logSrci.setup(KEY_SIZE, db);

    KwRange query = KwRange {3, 5};
    std::vector<Doc> results = logSrci.search(query);
    std::cout << "Results:";
    for (Doc idOp : results) {
        std::cout << " " << idOp;
    }
    std::cout << std::endl;
}
