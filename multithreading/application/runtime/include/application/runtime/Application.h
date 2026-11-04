#pragma once

#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

namespace multithreading::runtime {

    constexpr size_t APPLICATION_INFO_ALIGNMENT = 128;

    template <typename T>
    struct alignas(APPLICATION_INFO_ALIGNMENT) ApplicationInfo {
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
        explicit Application(ApplicationInfo<T> information) noexcept
            : appInformation(std::move(information)) {
        }

        std::optional<T> SafeStart(const std::function<T()>& task) noexcept {
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

                return taskResult;
            } catch (std::exception& exception) {
                std::cout << "Uncaught exception at " << appInformation.appName << ": "
                          << exception.what() << '\n';

                return std::nullopt;
            } catch (...) {
                std::cout << "Uncaught non-standard exception at " << appInformation.appName
                          << '\n';

                return std::nullopt;
            }
        }

        template <size_t N>
        std::array<std::optional<T>, N> SafeStartQueue(
            const std::array<std::function<T()>, N>& task
        ) noexcept {
            std::array<std::optional<T>, N> executionResults{};
            std::ranges::transform(
                task,
                executionResults.begin(),
                [this](const auto& single) {
                    return this->SafeStart(single);
                },
                std::identity{}
            );

            return executionResults;
        }
    };
}  // namespace multithreading::runtime
