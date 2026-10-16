#pragma once

#include <multithreading/utilities/include/performance/AlignedField.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <future>
#include <iterator>
#include <memory>
#include <optional>
#include <semaphore>
#include <utility>
#include <vector>

#include "./BoundedQueue.h"

namespace multithreading::structures::bounded_queue {

    // Sequence has 2 states:
    // 1) 'full' (lap_number * capacity) + 1: (i.e. capacity = 10, then 1, 11, 21, 31... indicate
    // 'full')
    // 2) 'empty' (lap_number * capacity): (i.e. capacity = 10, then 0, 10, 20, 30...
    // indicate 'empty')
    // Aligning a slot to its size rounded up to a power of two means no slot of a cache line or
    // less ever straddles two lines.
    template <typename T>
    constexpr size_t CIRCULAR_SLOT_ALIGNMENT =
        std::bit_ceil(sizeof(std::atomic<size_t>) + sizeof(std::optional<T>));

    template <typename T>
    struct alignas(CIRCULAR_SLOT_ALIGNMENT<T>) CircularSlot {
        std::atomic<size_t> sequence{0};
        std::optional<T> value;
    };

    template <typename T>
    class LockFreeBoundedQueueImpl {
    private:
        const size_t capacity;
        std::vector<CircularSlot<T>> data;
        std::counting_semaphore<> items_available;

        // Producers and consumers each spin on their own position, so keep the two on separate
        // cache lines.
        utilities::performance::AlignedField<std::atomic<size_t>> enqueue_pos;
        utilities::performance::AlignedField<std::atomic<size_t>> dequeue_pos;

        // Positions grow without bound and wrap onto the ring here, so the index is always valid.
        CircularSlot<T>& slot_at(const size_t position) noexcept {
            return *std::next(data.begin(), static_cast<ptrdiff_t>(position % capacity));
        }

        [[nodiscard]] const CircularSlot<T>& slot_at(const size_t position) const noexcept {
            return *std::next(data.begin(), static_cast<ptrdiff_t>(position % capacity));
        }

    public:
        explicit LockFreeBoundedQueueImpl(const size_t capacity)
            : capacity(capacity)
            // Explicit allocator: libstdc++ defaults it, which fuchsia-default-arguments-calls
            // flags.
            , data(capacity, std::allocator<CircularSlot<T>>{})
            , items_available(0)
            , enqueue_pos(0)
            , dequeue_pos(0) {
            for (size_t i = 0; i < capacity; ++i) {
                slot_at(i).sequence.store(i, std::memory_order_relaxed);
            }
        }

        LockFreeBoundedQueueImpl(LockFreeBoundedQueueImpl&&) = delete;
        LockFreeBoundedQueueImpl(const LockFreeBoundedQueueImpl&) = delete;
        LockFreeBoundedQueueImpl& operator=(const LockFreeBoundedQueueImpl&) = delete;
        LockFreeBoundedQueueImpl& operator=(LockFreeBoundedQueueImpl&&) = delete;

        ~LockFreeBoundedQueueImpl() = default;

        std::optional<T> try_dequeue() {
            while (true) {
                size_t position = dequeue_pos.get().load(std::memory_order_relaxed);
                CircularSlot<T>& slot = slot_at(position);
                const size_t sequence = slot.sequence.load(std::memory_order_acquire);

                if (sequence == position) {
                    // Sequence indicates 'empty' state. Probably means that our queue is empty.
                    return std::nullopt;
                }
                if (sequence != position + 1) {
                    // Sequence does not indicate 'Full' state, the slot is not ready to be read
                    // by consumer. Continue spinning.
                    continue;
                }

                // CAS to ensure that dequeue_pos didn't update while we were proceeding. This way
                // we will either claim the position or identify race condition loss and retry.
                if (dequeue_pos.get().compare_exchange_weak(
                        position,
                        position + 1,
                        std::memory_order_acquire,
                        std::memory_order_relaxed
                    )) {
                    // Position successfully claimed, proceed and return the value.
                    std::optional<T> result = std::exchange(slot.value, std::nullopt);
                    slot.sequence.store(position + capacity, std::memory_order_release);
                    return result;
                }
            }
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) {
            const auto deadline = std::chrono::steady_clock::now() + timeout;

            // We don't check the time equality inside the while loop as it is ID-based and
            // affects performance in a bad way.
            while (true) {
                const auto now_time = std::chrono::steady_clock::now();
                if (now_time >= deadline) {
                    // Time limit exceeded, break from cycle.
                    break;
                }

                // Wait for time left after previous iterations.
                if (items_available.try_acquire_for(deadline - now_time)) {
                    // If the value added to semaphore - try to dequeue it. It can still fail
                    // in a racing condition with common .try_dequeue() callers.
                    if (auto result = try_dequeue(); result.has_value()) {
                        // Dequeue succeeded - return the result of operation.
                        return result;
                    }
                } else {
                    // Time limit exceeded, break from cycle.
                    break;
                }
            }

            // Final attempt to dequeue the value.
            return try_dequeue();
        }

        bool try_enqueue(T&& value) {
            while (true) {
                size_t position = enqueue_pos.get().load(std::memory_order_relaxed);
                CircularSlot<T>& slot = slot_at(position);
                const size_t sequence = slot.sequence.load(std::memory_order_acquire);

                if (sequence < position) {
                    // The slot we are trying to address is still on the previous lap,
                    // which means that it is full and have not been read. It means that the
                    // queue is full
                    if (enqueue_pos.get().load(std::memory_order_acquire) == position) {
                        // Ensure atomic state didn't change and if it didn't return false.
                        return false;
                    }
                    // Atomic state changed, probably we've got new space to enqueue.
                    // Retry in next iteration.
                    continue;
                }
                if (sequence > position) {
                    // We lost racing condition and someone has claimed our slot before we did.
                    // Continue to get fresh enqueue position in next iteration.
                    continue;
                }

                // Slot is ready and in the correct 'Empty' state. Proceed with CAS operation
                // to ensure position didn't change since the beginning of the operation.
                if (enqueue_pos.get().compare_exchange_weak(
                        position,
                        position + 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed
                    )) {
                    slot.value.emplace(std::move(value));
                    slot.sequence.store(position + 1, std::memory_order_release);
                    items_available.release(1);
                    return true;
                }
            }
        }

        [[nodiscard]] bool is_empty() const {
            while (true) {
                const size_t position = dequeue_pos.get().load(std::memory_order_relaxed);
                const size_t sequence = slot_at(position).sequence.load(std::memory_order_acquire);

                if (position == dequeue_pos.get().load(std::memory_order_acquire)) {
                    // If the sequence equals to position, it means that Slot state is 'Empty',
                    // which means that the queue is empty.
                    return sequence == position;
                }
            }
        }

        [[nodiscard]] bool is_full() const {
            while (true) {
                const size_t position = enqueue_pos.get().load(std::memory_order_relaxed);
                const size_t sequence = slot_at(position).sequence.load(std::memory_order_acquire);

                if (position == enqueue_pos.get().load(std::memory_order_acquire)) {
                    // If the sequence is less than position, it means that our sequence is
                    // still on previous lap and wasn't read. Indicates that queue is full.
                    return sequence < position;
                }
            }
        }
    };

    template <typename T>
    class LockFreeBoundedQueue : public BoundedQueue<T> {
    private:
        LockFreeBoundedQueueImpl<T> impl;

    public:
        explicit LockFreeBoundedQueue(const size_t capacity)
            : BoundedQueue<T>()
            , impl(capacity) {
        }

        ~LockFreeBoundedQueue() override = default;

        LockFreeBoundedQueue(LockFreeBoundedQueue&&) = delete;
        LockFreeBoundedQueue(const LockFreeBoundedQueue&) = delete;
        LockFreeBoundedQueue& operator=(const LockFreeBoundedQueue&) = delete;
        LockFreeBoundedQueue& operator=(LockFreeBoundedQueue&&) = delete;

        std::optional<T> try_dequeue() override {
            return impl.try_dequeue();
        }

        std::optional<T> wait_dequeue(const std::chrono::steady_clock::duration& timeout) override {
            return impl.wait_dequeue(timeout);
        }

        std::future<std::optional<T>> wait_dequeue_async(
            const std::chrono::steady_clock::duration& timeout
        ) override {
            return std::async(std::launch::async, [this, timeout]() -> auto {
                return this->impl.wait_dequeue(timeout);
            });
        }

        bool try_enqueue(T&& value) override {
            return impl.try_enqueue(std::move(value));
        }

        [[nodiscard]] bool is_empty(bool /*isPrecise*/) const override {
            return impl.is_empty();
        }

        [[nodiscard]] bool is_full(bool /*isPrecise*/) const override {
            return impl.is_full();
        }
    };
}  // namespace multithreading::structures::bounded_queue