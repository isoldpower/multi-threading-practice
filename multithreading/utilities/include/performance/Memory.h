#pragma once

#include "../Units.h"
#include <sys/resource.h>
#include <iostream>

#ifdef __linux__
#include <fstream>
#include <string>
#elif __APPLE__
#include <mach/mach.h>
#endif

namespace multithreading::utilities::performance {

    class MemoryMeasurement {
    public:
        static double peakMemoryUsage() {
            struct rusage usage;
            getrusage(RUSAGE_SELF, &usage);
            double memory_usage = 0;
#ifdef __APPLE__
            memory_usage = static_cast<double>(usage.ru_maxrss) / static_cast<double>(KiB);
#else
            memory_usage = usage.ru_maxrss;
#endif
            return memory_usage;
        }

        static double currentMemoryUsage() {
#ifdef __linux__
            std::ifstream status_stream("/proc/self/status", std::ios_base::in);
            std::string line;
            while (std::getline(status_stream, line)) {
                if (line.find("VmRSS:") != std::string::npos) {
                    long rss;
                    sscanf(line.c_str(), "VmRSS: %ld kB", &rss);
                    return static_cast<size_t>(rss);
                }
            }

            std::cerr << "MemoryMeasurement::currentMemoryUsage() failed "
                         "to find the memory state in the /proc/self/status file\n";
            return 0;
# elif __APPLE__
            struct mach_task_basic_info basic_info;
            mach_msg_type_number_t msg_size = MACH_TASK_BASIC_INFO_COUNT;
            kern_return_t kerr = task_info(
                mach_task_self(),
                MACH_TASK_BASIC_INFO,
                (task_info_t)&basic_info,
                &msg_size
            );
            if (kerr == KERN_SUCCESS) {
                return static_cast<double>(basic_info.resident_size) / static_cast<double>(KiB);
            }

            std::cerr << "MemoryMeasurement::currentMemoryUsage() failed "
                         "to retrieve the memory state: " << kerr << '\n';
            return 0;
#else
#error "The platform is not supported for memory usage identification"
#endif
        }
    };
} // namespace multithreading::utilities::performance
