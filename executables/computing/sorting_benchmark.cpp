#include <iostream>
#include <memory>
#include <ostream>


auto main() -> int {
    try {
        std::cout << "Hello!\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';

        return -1;
    } catch (...) {
        std::cerr << "Unknown exception\n";

        return -1;
    }
}
