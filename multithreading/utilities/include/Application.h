#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <optional>

namespace multithreading::utilities {

    template <typename T>
    struct alignas(64) ApplicationInfo {
    public:
        std::string appName;
        std::string appVersion;
        std::optional<std::function<void()>> beforeTask;
        std::optional<std::function<void(T)>> afterTask;
    };

    template <typename T>
    class Application {
    private:
        ApplicationInfo<T> appInformation;
    public:
        explicit Application(ApplicationInfo<T> information)
            : appInformation(std::move(information))
        {}

        std::optional<T> SafeStart(std::function<T()> task) noexcept {
            try {
                const auto beforeTaskDecorator = appInformation.beforeTask;
                if (beforeTaskDecorator.has_value()) {
                    beforeTaskDecorator.value()();
                }

                T taskResult = task();

                const auto afterTaskDecorator = appInformation.afterTask;
                if (afterTaskDecorator.has_value()) {
                    afterTaskDecorator.value()(taskResult);
                }

                return 0;
            } catch (std::exception& exception) {
                const std::string prefix = "Uncaught exception at " + appInformation.appName;
                std::cout << prefix << ": " << exception.what() << '\n';

                return std::nullopt;
            }
        }

        template <size_t N>
        std::array<std::optional<T>, N> SafeStartQueue(
            std::array<std::function<T()>, N> task
        ) noexcept {
            std::array<std::optional<T>, N> executionResults{};

            for (size_t i = 0; i < N; i++) {
                std::optional<T> executionResult = this->SafeStart(task.at(i));
                executionResults.at(i) = executionResult;
            }

            return executionResults;
        }
    };
} // namespace multithreading::utilities
