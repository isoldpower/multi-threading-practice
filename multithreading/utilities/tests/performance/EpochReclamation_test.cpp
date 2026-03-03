#include <gtest/gtest.h>

#include "../../include/performance/EpochGuard.h"
#include "../../include/threads/CountThreadBarrier.h"
#include "../../include/threads/ThreadBarrier.h"

template <typename T>
struct TrackDeleted {
public:
    T* flag;

    ~TrackDeleted() {
        *flag = true;
    }
};

TEST(EpochReclamationTest, EngageAndFreeDoNotCrash) {
    multithreading::utilities::performance::EpochReclamation<int> reclamation;
    {
        const multithreading::utilities::performance::EpochGuard<int> guard(&reclamation);
    }
}

TEST(EpochReclamationTest, NestedGuardsOnSameThread) {
    multithreading::utilities::performance::EpochReclamation<int> reclamation;
    {
        const multithreading::utilities::performance::EpochGuard<int> guard_first(&reclamation);
        const multithreading::utilities::performance::EpochGuard<int> guard_second(&reclamation);
    }
}

TEST(EpochReclamationTest, RetiredNodeNotDeletedWhileGuardActive) {
    const multithreading::utilities::performance::EpochReclamation<int> reclamation;
    bool deleted = false;

    multithreading::utilities::performance::EpochReclamation<TrackDeleted<bool>> tracked_reclamation;
    auto* node = new TrackDeleted{&deleted};

    {
        const multithreading::utilities::performance::EpochGuard guard(&tracked_reclamation);
        tracked_reclamation.retire_reference(node);
        EXPECT_FALSE(deleted);
    }
}

TEST(EpochReclamationTest, RetiredNodeDeletedAfterEpochAdvances) {
    constexpr int RETIRE_LIST_SIZE = 10;
    const multithreading::utilities::performance::EpochReclamation<int> reclamation;
    std::vector<int> deleted_list(RETIRE_LIST_SIZE, 0);

    multithreading::utilities::performance::EpochReclamation<TrackDeleted<int>> tracked_reclamation;

    for (size_t i = 0; i < multithreading::utilities::performance::RECLAMATION_FREQUENCY * 3; ++i) {
        const multithreading::utilities::performance::EpochGuard guard(&tracked_reclamation);
        if (i < RETIRE_LIST_SIZE) {
            tracked_reclamation.retire_reference(new TrackDeleted{ &deleted_list.at(i) });
        }
    }

    const bool is_all_cleared = std::all_of(
        deleted_list.begin(),
        deleted_list.end(),
        [](bool deleted){ return deleted == 1; });
    EXPECT_TRUE(is_all_cleared);
}

TEST(EpochReclamationTest, MultipleThreadsGetDistinctSlots) {
    constexpr size_t THREAD_COUNT = 8;

    multithreading::utilities::performance::EpochReclamation<int> reclamation;
    multithreading::utilities::threads::CountThreadBarrier barrier(THREAD_COUNT);
    std::atomic<size_t> active_simultaneously{0};
    std::atomic<size_t> peak{0};
    std::vector<std::thread> threads;

    threads.reserve(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            const multithreading::utilities::performance::EpochGuard guard(&reclamation);
            const size_t current = active_simultaneously.fetch_add(1, std::memory_order_release) + 1;
            size_t prev = peak.load(std::memory_order_acquire);
            while (current > prev && !peak.compare_exchange_weak(
                prev,
                current,
                std::memory_order_release,
                std::memory_order_acquire
            )) {}

            barrier.enqueue_and_wait();
            active_simultaneously.fetch_sub(1, std::memory_order_release);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
    barrier.join();
    EXPECT_EQ(peak.load(std::memory_order_acquire), THREAD_COUNT);
}

TEST(EpochReclamationTest, ExceedingMaxThreadsThrows) {
    multithreading::utilities::performance::EpochReclamation<int> reclamation;
    std::vector<std::thread> threads;
    std::atomic<size_t> throw_count{0};
    std::mutex start_mu;
    std::condition_variable start_cv;

    bool start = false;
    threads.reserve(multithreading::utilities::performance::MAX_THREADS + 1);
    for (size_t i = 0; i < multithreading::utilities::performance::MAX_THREADS + 1; ++i) {
        threads.emplace_back([&]() {
            try {
                const multithreading::utilities::performance::EpochGuard guard(&reclamation);
                std::unique_lock lock(start_mu);
                start_cv.wait(lock, [&] {
                    return start;
                });
            } catch (const std::runtime_error&) {
                throw_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    {
        const std::scoped_lock lock(start_mu);
        start = true;
    }

    start_cv.notify_all();
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(throw_count.load(std::memory_order_acquire), 1);
}

TEST(EpochReclamationTest, ConcurrentRetireAndReclaimNoDoubleFree) {
    multithreading::utilities::performance::EpochReclamation<int> reclamation;
    constexpr size_t THREAD_COUNT = 8;
    constexpr size_t OPS_PER_THREAD = 1000;
    std::atomic<size_t> retire_count{0};

    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&]() {
            for (size_t j = 0; j < OPS_PER_THREAD; ++j) {
                const multithreading::utilities::performance::EpochGuard guard(&reclamation);
                reclamation.retire_reference(new int(static_cast<int>(j)));
                retire_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(retire_count.load(std::memory_order_acquire), THREAD_COUNT * OPS_PER_THREAD);
}

TEST(EpochReclamationTest, EpochDoesNotAdvanceWhileThreadActive) {
    multithreading::utilities::performance::EpochReclamation<TrackDeleted<bool>> tracked_reclamation;

    bool deleted = false;
    {
        const multithreading::utilities::performance::EpochGuard guard(&tracked_reclamation);
        tracked_reclamation.retire_reference(new TrackDeleted{ &deleted });

        std::thread background([&]() {
            for (size_t i = 0; i < multithreading::utilities::performance::RECLAMATION_FREQUENCY * 3; ++i) {
                const multithreading::utilities::performance::EpochGuard bg_guard(&tracked_reclamation);
            }
        });
        background.join();

        EXPECT_FALSE(deleted);
    }

    std::thread cleanup([&]() {
        for (size_t i = 0; i < multithreading::utilities::performance::RECLAMATION_FREQUENCY * 3; ++i) {
            const multithreading::utilities::performance::EpochGuard bg_guard(&tracked_reclamation);
        }
    });
    cleanup.join();

    EXPECT_TRUE(deleted);
}