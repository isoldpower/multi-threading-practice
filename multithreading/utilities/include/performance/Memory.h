#pragma once

#include <sys/resource.h>

#include <iostream>

#include "../Units.h"

#ifdef __linux__
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#elif __APPLE__
#include <mach/mach.h>
#endif

namespace multithreading::utilities::performance {

    class MemoryMeasurement {
    public:
        static double peakMemoryUsage() {
            struct rusage usage{};
            getrusage(RUSAGE_SELF, &usage);
            double memory_usage = 0;
#ifdef __APPLE__
            memory_usage = static_cast<double>(usage.ru_maxrss) / static_cast<double>(KiB);
#else
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            memory_usage = static_cast<double>(usage.ru_maxrss);
            memory_usage = std::max(memory_usage, currentMemoryUsage());
#endif
            return memory_usage;
        }

        static double currentMemoryUsage() {
#ifdef __linux__
            std::ifstream status_stream("/proc/self/status", std::ios_base::in);
            std::string line;
            while (std::getline(status_stream, line)) {
                if (line.starts_with("VmRSS:")) {
                    std::istringstream fields(line, std::ios_base::in);
                    std::string label;
                    size_t rss_kib = 0;
                    fields >> label >> rss_kib;
                    return static_cast<double>(rss_kib);
                }
            }

            std::cerr << "MemoryMeasurement::currentMemoryUsage() failed "
                         "to find the memory state in the /proc/self/status file\n";
            return 0;
#elif __APPLE__
            struct mach_task_basic_info basic_info{};
            mach_msg_type_number_t msg_size = MACH_TASK_BASIC_INFO_COUNT;
            const kern_return_t kerr = task_info(
                mach_task_self(),
                MACH_TASK_BASIC_INFO,
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                reinterpret_cast<task_info_t>(&basic_info),
                &msg_size
            );
            if (kerr == KERN_SUCCESS) {
                return static_cast<double>(basic_info.resident_size) / static_cast<double>(KiB);
            }

            std::cerr << "MemoryMeasurement::currentMemoryUsage() failed "
                         "to retrieve the memory state: "
                      << kerr << '\n';
            return 0;
#else
#error "The platform is not supported for memory usage identification"
#endif
        }
    };
}  // namespace multithreading::utilities::performance
