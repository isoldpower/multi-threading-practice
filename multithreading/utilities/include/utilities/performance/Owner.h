#pragma once

#include <type_traits>

namespace gsl {
    /** @brief A raw pointer that owns its pointee and is responsible for deleting it.
     * Purely an annotation: it is the pointer type itself, so it costs nothing at runtime. */
    template <typename T>
        requires std::is_pointer_v<T>
    using owner = T;
}  // namespace gsl
