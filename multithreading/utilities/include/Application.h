#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>

namespace multithreading::utilities {

    struct alignas(64) ApplicationInfo {
    public:
        std::string appName;
        std::string appVersion;
    };

    template <typename T>
    class Application {
    private:
        ApplicationInfo appInformation;
    public:
        explicit Application(ApplicationInfo  information)
            : appInformation(std::move(information))
        {}

        std::optional<T> SafeStart(std::function<T()> task) noexcept {
            try {
                return task();
            } catch (std::exception& exception) {
                const std::string prefix = "Uncaught exception at " + appInformation.appName;
                std::cout << prefix << ": " << exception.what() << '\n';

                return std::nullopt;
            }
        }
    };

} // namespace multithreading::utilities
