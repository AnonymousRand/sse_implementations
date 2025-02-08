// temp compilation `g++ main.cpp util.cpp pi_bas.cpp log_src.cpp tdag.cpp -lcrypto -o a`
#include <cmath>
#include <random>

#include "log_src.h"
#include "pi_bas.h"
#include "tdag.h" // temp

void exp1(PiBasClient client, PiBasServer server) {
    client.setup(KEY_SIZE);
    server.setEncIndex(client.buildIndex());
    // query
    KwRange kwRange = KwRange {12, 12};
    QueryToken queryToken = client.trpdr(kwRange);
    std::vector<Id> results = server.search(queryToken);
    std::cout << "Querying " << kwRange << ": results are";
    for (Id result : results) {
        std::cout << " " << result;
    }
    std::cout << std::endl;
}

int main() {
    // experiment 1: db of size 2^20 and vary range sizes
    Db db;
    const int dbSize = pow(2, 3);
    std::random_device dev;
    std::mt19937 rand(dev());
    std::uniform_int_distribution<int> dist(0, dbSize - 1);
    std::cout << "----- Dataset -----" << std::endl;
    for (int i = 0; i < dbSize; i++) {
        Kw kw = dist(rand);
        db[i] = KwRange {kw, kw};
        std::cout << i << ": " << db[i] << std::endl;
    }

    //PiBasClient piBasClient = PiBasClient(db);
    //PiBasServer piBasServer = PiBasServer();
    //exp1(piBasClient, piBasServer);

    LogSrcClient logSrcClient = LogSrcClient(db);
    LogSrcServer logSrcServer = LogSrcServer();
    logSrcClient.setup(KEY_SIZE);
    logSrcClient.buildIndex();
    //exp1(logSrcClient, logSrcServer);

    // experiment 2: fixed range size and vary db sizes
}
