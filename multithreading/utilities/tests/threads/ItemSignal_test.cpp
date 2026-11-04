#include <gtest/gtest.h>
#include <utilities/threads/ItemSignal.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using multithreading::utilities::threads::ItemSignal;

namespace {
    /** A plain locked deque, so the tests exercise the signal and nothing else. */
    class SignalledDeque {
    private:
        std::mutex items_mutex;
        std::deque<int> items;
        ItemSignal signal;

    public:
        void push(const int value) {
            {
                const std::scoped_lock lock(items_mutex);
                items.push_back(value);
            }
            signal.notify();
        }

        std::optional<int> try_pop() {
            const std::scoped_lock lock(items_mutex);
            if (items.empty()) {
                return std::nullopt;
            }
            const int value = items.front();
            items.pop_front();
            return value;
        }

        std::optional<int> wait_pop(const std::chrono::steady_clock::duration& timeout) {
            return signal.wait_for(timeout, [this]() {
                return try_pop();
            });
        }
    };
}  // namespace

TEST(ItemSignalTest, TimesOutWhenNothingArrives) {
    SignalledDeque deque;
    const auto started = std::chrono::steady_clock::now();

    EXPECT_FALSE(deque.wait_pop(std::chrono::milliseconds(50)).has_value());
    EXPECT_GE(std::chrono::steady_clock::now() - started, std::chrono::milliseconds(50));
}

TEST(ItemSignalTest, ReturnsAnItemAlreadyThere) {
    SignalledDeque deque;
    deque.push(7);

    EXPECT_EQ(deque.wait_pop(std::chrono::milliseconds(0)), 7);
}

TEST(ItemSignalTest, WakesWhenAnItemArrives) {
    SignalledDeque deque;
    std::thread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        deque.push(42);
    });

    const auto started = std::chrono::steady_clock::now();
    EXPECT_EQ(deque.wait_pop(std::chrono::seconds(10)), 42);
    EXPECT_LT(std::chrono::steady_clock::now() - started, std::chrono::seconds(5));
    producer.join();
}

TEST(ItemSignalTest, NoWakeUpIsLostUnderContention) {
    // A lost wake-up leaves a consumer asleep until its timeout, which then returns empty
    constexpr size_t PRODUCERS = 4;
    constexpr size_t CONSUMERS = 4;
    constexpr size_t ITEMS_PER_PRODUCER = 5000;
    SignalledDeque deque;
    std::atomic<size_t> received{0};
    std::atomic<size_t> timed_out{0};

    std::vector<std::thread> threads;
    threads.reserve(PRODUCERS + CONSUMERS);
    for (size_t i = 0; i < CONSUMERS; ++i) {
        threads.emplace_back([&]() {
            for (size_t j = 0; j < PRODUCERS * ITEMS_PER_PRODUCER / CONSUMERS; ++j) {
                if (deque.wait_pop(std::chrono::seconds(5)).has_value()) {
                    received.fetch_add(1, std::memory_order_relaxed);
                } else {
                    timed_out.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (size_t i = 0; i < PRODUCERS; ++i) {
        threads.emplace_back([&]() {
            for (size_t j = 0; j < ITEMS_PER_PRODUCER; ++j) {
                deque.push(static_cast<int>(j));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(timed_out.load(), 0U);
    EXPECT_EQ(received.load(), PRODUCERS * ITEMS_PER_PRODUCER);
}
