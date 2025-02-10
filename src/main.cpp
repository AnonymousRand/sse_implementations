// temp compilation `g++ main.cpp pi_bas.cpp log_src.cpp tdag.cpp util.cpp -lcrypto -o a`
#include <cmath>
#include <random>
#include <type_traits>

#include "log_src.h"
#include "pi_bas.h"

void exp1(SseClient<ustring, EncInd>& client, SseServer<EncInd>& server, Db db) {
    ustring key = client.setup(KEY_SIZE);
    std::cout << "Building index..." << std::endl;
    EncInd encInd = client.buildIndex(key, db);

    KwRange query = KwRange(2, 4);
    std::cout << "Querying " << query << "..." << std::endl;
    QueryToken queryToken = client.trpdr(key, query);
    std::vector<Id> results = server.search(encInd, queryToken);
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

    PiBasClient piBasClient = PiBasClient();
    PiBasServer piBasServer = PiBasServer();
    exp1(piBasClient, piBasServer, db);

    //LogSrcClient logSrcClient = LogSrcClient(db);
    //LogSrcServer logSrcServer = LogSrcServer();
    //exp1(logSrcClient, logSrcServer);

    // experiment 2: fixed range size and vary db sizes
}
