// #include <gtest/gtest.h>
//
// #include <chrono>
// #include <cstdint>
// #include <map>
// #include <memory>
// #include <optional>
// #include <thread>
// #include <vector>
// #include <multithreading/utilities/include/threads/ThreadBarrier.h>
//
// #include "../../include/bounded_queue/FGLockBoundedQueue.h"
// #include "../../include/bounded_queue/LockFreeBoundedQueue.h"
//
// constexpr size_t DEFAULT_QUEUE_SIZE = 10;
//
// template <typename T>
// class BoundedQueueTest : public ::testing::Test {
// protected:
//     std::unique_ptr<T> queue;
//
//     void SetUp() override {
//         queue = std::make_unique<T>(DEFAULT_QUEUE_SIZE);
//     }
//
//     void TearDown() override {
//         queue = nullptr;
//     }
// };
//
// using BoundedQueueImplementations = ::testing::Types<
//     multithreading::structures::bounded_queue::FGLockBoundedQueue<int>,
//     multithreading::structures::bounded_queue::LockFreeBoundedQueue<int>
// >;
// TYPED_TEST_SUITE(BoundedQueueTest, BoundedQueueImplementations);
//
// TYPED_TEST(BoundedQueueTest, StartsEmpty) {
//     EXPECT_TRUE(this->queue->is_empty(false));
// }
//
// TYPED_TEST(BoundedQueueTest, StartsEmptyPrecise) {
//     EXPECT_TRUE(this->queue->is_empty(true));
// }
//
// TYPED_TEST(BoundedQueueTest, EmptyDequeueNullopt) {
//     const std::optional<int> dequeued_value = this->queue->try_dequeue();
//     EXPECT_FALSE(dequeued_value.has_value());
// }
//
// TYPED_TEST(BoundedQueueTest, EnqueueElementSuccess) {
//     EXPECT_TRUE(this->queue->try_enqueue(1));
// }
//
// TYPED_TEST(BoundedQueueTest, SizeReflectsEnqueue) {
//     EXPECT_TRUE(this->queue->try_enqueue(1));
//     EXPECT_FALSE(this->queue->is_empty(false));
// }
//
// TYPED_TEST(BoundedQueueTest, FilledQueueIsFull) {
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         EXPECT_TRUE(this->queue->try_enqueue(i));
//     }
//
//     EXPECT_FALSE(this->queue->is_empty(false));
//     EXPECT_TRUE(this->queue->is_full(false));
// }
//
// TYPED_TEST(BoundedQueueTest, DequeueNotEmpty) {
//     EXPECT_TRUE(this->queue->try_enqueue(1));
//
//     const std::optional<int> dequeued_value = this->queue->try_dequeue();
//     EXPECT_TRUE(dequeued_value.has_value());
// }
//
// TYPED_TEST(BoundedQueueTest, FifoOperationsOrder) {
//     constexpr size_t TEST_SIZE = 2;
//     for (size_t i = 0; i < TEST_SIZE; ++i) {
//         EXPECT_TRUE(this->queue->try_enqueue(i));
//     }
//
//     for (size_t i = 0; i < TEST_SIZE; ++i) {
//         const std::optional<int> dequeued_value = this->queue->try_dequeue();
//         EXPECT_EQ(dequeued_value.value(), i);
//     }
// }
//
// TYPED_TEST(BoundedQueueTest, FullQueueConsistent) {
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         EXPECT_TRUE(this->queue->try_enqueue(i));
//     }
//     EXPECT_FALSE(this->queue->try_enqueue(-1));
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         const std::optional<int> dequeued_value = this->queue->try_dequeue();
//         EXPECT_EQ(dequeued_value.value(), i);
//     }
// }
//
// TYPED_TEST(BoundedQueueTest, FreeAndRefill) {
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         EXPECT_TRUE(this->queue->try_enqueue(i));
//     }
//
//     const std::optional<int> dequeued_value = this->queue->try_dequeue();
//     EXPECT_EQ(dequeued_value.value(), 0);
//
//     EXPECT_TRUE(this->queue->try_enqueue(DEFAULT_QUEUE_SIZE));
//     EXPECT_TRUE(this->queue->is_full(false));
// }
//
// TYPED_TEST(BoundedQueueTest, WaitDequeueTimeout) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
//
//     const auto dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
//     EXPECT_FALSE(dequeue_value.has_value());
// }
//
// TYPED_TEST(BoundedQueueTest, AsyncWaitDequeueTriggers) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
//
//     std::future<std::optional<int>> dequeue_future = this->queue->wait_dequeue_async(TEST_DURATION);
//     std::this_thread::sleep_for(TEST_DURATION / 2);
//     this->queue->try_enqueue(0);
//     const std::optional<int> dequeue_value = dequeue_future.get();
//
//     EXPECT_TRUE(dequeue_value.has_value());
//     EXPECT_TRUE(this->queue->is_empty(false));
// }
//
// TYPED_TEST(BoundedQueueTest, WaitDequeueTriggers) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
//
//     std::optional<int> dequeue_value;
//     auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
//         dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
//     });
//
//     std::this_thread::sleep_for(TEST_DURATION / 2);
//     this->queue->try_enqueue(0);
//     dequeue_thread.join();
//
//     EXPECT_TRUE(dequeue_value.has_value());
// }
//
// TYPED_TEST(BoundedQueueTest, EnqueueAfterWaitDequeueTimeout) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
//     constexpr double TIMEOUT_OFFSET = 0.1;
//
//     std::optional<int> dequeue_value;
//     auto dequeue_thread = std::thread([this, TEST_DURATION, &dequeue_value]() {
//         dequeue_value = this->queue->wait_dequeue(TEST_DURATION);
//     });
//
//     std::this_thread::sleep_for(TEST_DURATION * (1 + TIMEOUT_OFFSET));
//     this->queue->try_enqueue(0);
//     dequeue_thread.join();
//
//     EXPECT_FALSE(dequeue_value.has_value());
// }
//
// TYPED_TEST(BoundedQueueTest, HighContentionEnqueueResolution) {
//     std::map<int, size_t> elements_occurrences;
//     std::mutex occurrences_mutex;
//     multithreading::utilities::threads::ThreadBarrier barrier;
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         barrier.enqueue([i, this, &elements_occurrences, &occurrences_mutex]() {
//             if (this->queue->try_enqueue(i)) {
//                 const std::scoped_lock guard(occurrences_mutex);
//                 if (!elements_occurrences.contains(i)) {
//                     elements_occurrences.emplace(i, 1);
//                 } else {
//                     const size_t occurrences = elements_occurrences.at(i);
//                     elements_occurrences.at(i) = occurrences + 1;
//                 }
//             }
//         });
//     }
//
//     barrier.kickstart();
//     barrier.join();
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         const int casted_i = static_cast<int>(i);
//         const size_t occurrences = elements_occurrences.at(casted_i);
//
//         EXPECT_TRUE(this->queue->try_dequeue().has_value());
//         EXPECT_EQ(occurrences, 1);
//         elements_occurrences.erase(casted_i);
//     }
//
//     EXPECT_EQ(elements_occurrences.size(), 0);
// }
//
// TYPED_TEST(BoundedQueueTest, HighContentionDequeueResolution) {
//     std::map<int, size_t> elements_occurrences;
//     std::mutex occurrences_mutex;
//     multithreading::utilities::threads::ThreadBarrier barrier;
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         EXPECT_TRUE(this->queue->try_enqueue(i));
//
//         if (!elements_occurrences.contains(i)) {
//             elements_occurrences.emplace(i, 1);
//         } else {
//             const size_t occurrences = elements_occurrences.at(i);
//             elements_occurrences.at(i) = occurrences + 1;
//         }
//     }
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         barrier.enqueue([this, &elements_occurrences, &occurrences_mutex]() {
//             if (const auto dequeued_value = this->queue->try_dequeue(); dequeued_value.has_value()) {
//                 const size_t value = dequeued_value.value();
//                 const std::scoped_lock guard(occurrences_mutex);
//
//                 if (elements_occurrences.contains(value) && elements_occurrences.at(value) == 1) {
//                     elements_occurrences.erase(value);
//                 }
//             }
//         });
//     }
//
//     barrier.kickstart();
//     barrier.join();
//
//     EXPECT_EQ(elements_occurrences.size(), 0);
//     EXPECT_FALSE(this->queue->try_dequeue());
//     EXPECT_TRUE(this->queue->is_empty(false));
// }
//
// TYPED_TEST(BoundedQueueTest, HighContentionMCMPResolution) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::seconds(1);
//     std::atomic<size_t> enqueue_count = 0;
//     std::atomic<size_t> dequeue_count = 0;
//     multithreading::utilities::threads::ThreadBarrier enqueue_barrier;
//     multithreading::utilities::threads::ThreadBarrier dequeue_barrier;
//
//     for (size_t i = 0; i < DEFAULT_QUEUE_SIZE; ++i) {
//         enqueue_barrier.enqueue([i, this, &enqueue_count]() {
//             if (this->queue->try_enqueue(static_cast<int>(i))) {
//                 enqueue_count.fetch_add(1, std::memory_order_relaxed);
//             }
//         });
//         dequeue_barrier.enqueue([this, &dequeue_count, TEST_DURATION]() {
//             if (this->queue->wait_dequeue(TEST_DURATION).has_value()) {
//                 dequeue_count.fetch_add(1, std::memory_order_relaxed);
//             }
//         });
//     }
//
//     enqueue_barrier.kickstart();
//     dequeue_barrier.kickstart();
//
//     enqueue_barrier.join();
//     dequeue_barrier.join();
//
//     EXPECT_EQ(
//         enqueue_count.load(std::memory_order_acquire),
//         dequeue_count.load(std::memory_order_acquire)
//     );
//     EXPECT_TRUE(this->queue->is_empty(false));
// }
//
// TYPED_TEST(BoundedQueueTest, SevereContentionMCMPResolution) {
//     constexpr std::chrono::duration TEST_DURATION = std::chrono::milliseconds(100);
//     constexpr size_t LARGE_QUEUE_SIZE = DEFAULT_QUEUE_SIZE * 10;
//     constexpr size_t THREADS_COUNT = 8;
//     auto large_queue = std::make_unique<TypeParam>(LARGE_QUEUE_SIZE);
//
//     std::atomic<size_t> enqueue_count = 0;
//     std::atomic<size_t> dequeue_count = 0;
//     multithreading::utilities::threads::ThreadBarrier barrier;
//
//     for (size_t i = 0; i < THREADS_COUNT; ++i) {
//         barrier.enqueue([i, &large_queue, &enqueue_count]() {
//             for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
//                 if (large_queue->try_enqueue(static_cast<int>(j))) {
//                     enqueue_count.fetch_add(1, std::memory_order_relaxed);
//                 }
//             }
//         });
//         barrier.enqueue([&large_queue, &dequeue_count, TEST_DURATION]() {
//             for (size_t j = 0; j < LARGE_QUEUE_SIZE; ++j) {
//                 if (large_queue->wait_dequeue(TEST_DURATION).has_value()) {
//                     dequeue_count.fetch_add(1, std::memory_order_relaxed);
//                 }
//             }
//         });
//     }
//
//     barrier.kickstart();
//     barrier.join();
//
//     EXPECT_EQ(
//         enqueue_count.load(std::memory_order_acquire),
//         dequeue_count.load(std::memory_order_acquire)
//     );
//     EXPECT_TRUE(large_queue->is_empty(false));
// }