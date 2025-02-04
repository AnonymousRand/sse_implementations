#include "pi_bas.h"

#include <iostream>


void exp1(int secParam, PiBasClient client, PiBasServer server) {
    client.setup(secParam);
}


int main() {
    int secParam;
    std::cout << "Enter security parameter (key length): ";
    std::cin >> secParam;
    if (std::cin.fail() || secParam <= 0) {
        std::cerr << "Error: please enter a positive integer" << std::endl;
        exit(EXIT_FAILURE);
    }

    // PiBas experiements
    PiBasClient piBasClient = PiBasClient();
    PiBasServer piBasServer = PiBasServer();
    exp1(secParam, piBasClient, piBasServer);
}
