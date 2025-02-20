#include <cmath>

#include "log_src.h"
#include "pi_bas.h"

Db<> createDb(int dbSize) {
    Db<> db;
    for (int i = 0; i < dbSize; i++) {
        Kw kw = i;
        db.push_back(std::make_tuple(Id(i), KwRange {kw, kw}));
    }
    sortDb(db);
    return db;
}

int main() {
    int maxDbSizeExp;
    std::cout << "Enter database size (power of 2): ";
    std::cin >> maxDbSizeExp;
    int maxDbSize = pow(2, maxDbSizeExp);
    Db<> db = createDb(maxDbSize);

    PiBas<> piBas;
    LogSrc<> logSrc(piBas);
    logSrc.setup(KEY_SIZE, db);

    KwRange query = KwRange {3, 5};
    std::vector<Id> results = logSrc.search(query);
    std::cout << "Results:";
    for (Id id : results) {
        std::cout << " " << id;
    }
    std::cout << std::endl;
}
