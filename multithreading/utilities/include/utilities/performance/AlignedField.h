#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace multithreading::utilities::performance {

    constexpr size_t CACHE_LINE_SIZE = 64;

    /** @brief Holds a value on its own cache line(s) so neighbouring fields never share one.
     * Aligning the member also rounds sizeof up to a multiple of the alignment, so no explicit
     * tail padding is needed. */
    template <typename T, size_t NAlignas = CACHE_LINE_SIZE>
    class AlignedField {
    private:
        alignas(NAlignas) T value{};

    public:
        AlignedField() = default;

        template <typename... Args>
        explicit AlignedField(std::in_place_t /*tag*/, Args&&... args)
            : value(std::forward<Args>(args)...) {
        }

        template <typename U>
            requires(!std::same_as<std::decay_t<U>, AlignedField>)
        explicit AlignedField(U&& initial)
            : value(std::forward<U>(initial)) {
        }

        [[nodiscard]] T& get() noexcept {
            return value;
        }

        [[nodiscard]] const T& get() const noexcept {
            return value;
        }
    };
}  // namespace multithreading::utilities::performance
