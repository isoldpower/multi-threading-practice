#include "named_tools.h"


namespace cpp_structure::tools {
    NamedTools::NamedTools(std::string name)
        : name(std::move(name))
    {}

    void NamedTools::printHello() const {
        std::cout << "Hello, " << name << "!" << std::endl;
    }
}
