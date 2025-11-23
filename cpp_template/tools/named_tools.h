#pragma once
#include <iostream>

namespace cpp_structure::tools {
    class NamedTools {
        private:
            std::string name;
        public:
            explicit NamedTools(std::string name);
            ~NamedTools() = default;
            void printHello() const;
    };
}