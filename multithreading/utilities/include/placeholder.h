#pragma once

#include <string>


namespace multithreading::utilities {
    class Placeholder {
    public:
        Placeholder() = default;
        Placeholder(const Placeholder&) = delete;
        Placeholder& operator=(const Placeholder&) = delete;
        Placeholder(Placeholder&&) = delete;
        Placeholder& operator=(Placeholder&&) = delete;
        ~Placeholder() = default;

         static void SayHello(const std::string& name);
    };
} // namespace multithreading::utilities