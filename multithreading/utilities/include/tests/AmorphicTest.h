#pragma once

#include <gtest/gtest.h>
#include <type_traits>

namespace multithreading::utilities::tests {

    template <typename Queue>
    struct QueueValueType;

    template <template<typename> class Queue, typename T>
    struct QueueValueType<Queue<T>> {
        using type = T;
    };

    template <template<typename, typename...> class Queue, typename T, typename... Rest>
    struct QueueValueType<Queue<T, Rest...>> {
        using type = T;
    };

    template <typename T>
    class AmorphicTest : public ::testing::Test {
    protected:
        using ValueType = typename QueueValueType<T>::type;

        ValueType make_value(int val) {
            if constexpr (std::is_same_v<ValueType, std::unique_ptr<int>>) {
                return std::make_unique<int>(val);
            } else {
                return static_cast<ValueType>(val);
            }
        }

        bool are_equal_raw(const ValueType& lhs, const ValueType& rhs) {
            if constexpr (std::is_same_v<ValueType, std::unique_ptr<int>>) {
                return *lhs == *rhs;
            } else {
                return lhs == rhs;
            }
        }

        bool are_equal(const ValueType& lhs, const std::optional<ValueType>& value) {
            if (!value.has_value()) {
                return false;
            }

            return this->are_equal_raw(lhs, value.value());
        }

        int to_raw(const ValueType& value) {
            if constexpr (std::is_same_v<ValueType, std::unique_ptr<int>>) {
                return *value;
            } else {
                return value;
            }
        }
    };
} // namespace multithreading::utilities::tests