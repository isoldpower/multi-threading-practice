#pragma once

#include <cstdint>


namespace multithreading::utilities::performance {

    template <typename T, size_t NAlignas = 64>
    struct AlignedField {
        alignas(NAlignas) T value;

        static constexpr size_t padding_size = (NAlignas - (sizeof(T) % NAlignas)) % NAlignas;
        std::array<char, padding_size> padding;

        AlignedField() = default;
        template <typename... Args>
        explicit AlignedField(std::in_place_t, Args&&... args)
            : value(std::forward<Args>(args)...)
        {}

        template <
            typename U,
            typename = std::enable_if_t<!std::is_same_v<std::decay_t<U>, AlignedField>>
        >
        explicit AlignedField(U&& u)
            : value(std::forward<U>(u))
        {}

        T& operator*() { return value; }
        const T& operator*() const { return value; }

        T* operator->() { return &value; }
        const T* operator->() const { return &value; }

        operator T&() { return value; }
        operator const T&() const { return value; }
    };
} // namespace multithreading::utilities::performance
