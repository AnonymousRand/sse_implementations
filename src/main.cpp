#include "pi_bas.h"
#include "util.h" // temp

#include <cmath>
#include <iostream>
#include <random>


void exp1(int secParam, PiBasClient client, PiBasServer server) {
    client.setup(secParam);
}


int main() {
    int secParam;
    std::cout << "Enter security parameter/key length (must be 128, 192, or 256): ";
    std::cin >> secParam;
    if (std::cin.fail() || !(secParam == 128 || secParam == 192 || secParam == 256)) {
        std::cerr << "Error: invalid input" << std::endl;
        exit(EXIT_FAILURE);
    }

    // experiment 1: db of size 2^20 and vary range sizes
    Db db;
    std::random_device dev;
    std::mt19937 rand(dev());
    std::uniform_int_distribution<int> dist(pow(2, 20));
    for (int i = 0; i < pow(2, 20); i++) {
        db[i] = dist(rand);
    }

    PiBasClient piBasClient = PiBasClient(db);
    PiBasServer piBasServer = PiBasServer();
    //exp1(secParam, piBasClient, piBasServer);

    // experiment 2: fixed range size and vary db sizes
    
    // temp
    std::cout << std::get<0>(AesEncrypt(strToUCharPtr("hi"), 2, strToUCharPtr("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"), 128)) << std::endl;
}
