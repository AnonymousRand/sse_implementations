#include <cmath>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"

Db<> createDb(int dbSize) {
    Db<> db;
    for (int i = 0; i < dbSize; i++) {
        Kw kw = i;
        db.push_back(std::pair {IdOp(i), KwRange {kw, kw}});
    }
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
    std::vector<IdOp> results = logSrci.search(query);
    std::cout << "Results:";
    for (IdOp doc : results) {
        std::cout << " " << doc;
    }
    std::cout << std::endl;
}
