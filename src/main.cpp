// temp compilation `g++ main.cpp sse.cpp pi_bas.cpp log_src.cpp tdag.cpp util.cpp -lcrypto -o a`
#include <cmath>
#include <random>

#include "log_src.h"
#include "log_srci.h"
#include "pi_bas.h"
#include "sse.h"

template <typename ClientType, typename ServerType>
void exp1(ClientType& client, ServerType& server) {
    client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    server.setEncInd(client.buildIndex());

    KwRange query = KwRange(2, 4);
    std::cout << "Querying " << query << "..." << std::endl;
    std::vector<Id> results = server.template query<>(client, query);
    std::cout << "Results are:";
    for (Id result : results) {
        std::cout << " " << result;
    }
    std::cout << std::endl;
}

int main() {
    // experiment 1: db of size 2^20 and vary range sizes
    Db db;
    int dbSize = pow(2, 3);
    //std::random_device dev;
    //std::mt19937 rng(dev());
    //std::uniform_int_distribution<int> dist(0, dbSize - 1);
    //std::cout << "----- Dataset -----" << std::endl;
    for (int i = 0; i < dbSize; i++) {
        //Kw kw = dist(rng);
        Kw kw = i;
        db.insert(std::make_pair(i, KwRange(kw, kw)));
        //std::cout << i << ": " << db.find(i)->second << std::endl;
    }

    PiBasClient piBasClient = PiBasClient(db);
    PiBasServer piBasServer = PiBasServer();
    //exp1(piBasClient, piBasServer);

    //LogSrcClient logSrcClient = LogSrcClient(db);
    //LogSrcServer logSrcServer = LogSrcServer();
    //exp1(logSrcClient, logSrcServer);

    // experiment 2: fixed range size and vary db sizes
}
