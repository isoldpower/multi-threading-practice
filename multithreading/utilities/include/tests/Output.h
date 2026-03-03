#pragma once

#include <gtest/internal/gtest-port.h>

namespace multithreading::utilities::tests {

    template <typename TCallable>
    static void suppress_stdout(TCallable callable) {
        testing::internal::CaptureStdout();
        callable();
        testing::internal::GetCapturedStdout();
    }
} // namespace multithreading::utilities::tests