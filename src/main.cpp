#include <exception>
#include <iostream>
#include <string>

#include "server.hpp"

int main() {
    try {
        monitoring::Server server{"0.0.0.0", 8080};
        server.start();

        std::cout << "System monitoring server is listening on ws://127.0.0.1:8080\n"
                  << "Press Enter to stop the server.\n";

        std::string line;
        std::getline(std::cin, line);
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
