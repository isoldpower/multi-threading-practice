#include "../include/placeholder.h"

#include <string>
#include <iostream>
#include <ostream>


namespace multithreading::utilities {

    void Placeholder::SayHello(const std::string &name) {
        std::cout << "Hello, world! from " << name << '\n';
    }
} // namespace multithreading::utilities