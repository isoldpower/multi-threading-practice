#include "../include/placeholder.h"

#include <iostream>
#include <ostream>


namespace multithreading::utilities {

    void Placeholder::SayHello(const std::string &name) {
        std::cout << "Hello, world! from " << name << std::endl;
    }
} // namespace multithreading::utilities