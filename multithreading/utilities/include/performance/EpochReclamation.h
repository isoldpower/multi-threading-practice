#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "./Owner.h"

namespace multithreading::utilities::performance {

    constexpr size_t EPOCH_WINDOW_SIZE = 2;
    constexpr size_t MAX_THREADS = 128;
    constexpr size_t INACTIVE_SIGN = static_cast<size_t>(-1);
    constexpr size_t RETIRE_CLUSTER = 8;
    constexpr size_t RECLAMATION_FREQUENCY = 100;
    constexpr size_t THREAD_SLOT_ALIGNMENT = 16;
    constexpr size_t THREAD_RECORD_ALIGNMENT = 16;
    constexpr size_t RETIRE_LIST_ALIGNMENT = 128;

    struct alignas(THREAD_SLOT_ALIGNMENT) ThreadSlot {
        std::atomic<size_t> local_epoch{INACTIVE_SIGN};
        std::atomic<bool> is_used{false};
    };

    template <typename T>
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct alignas(RETIRE_LIST_ALIGNMENT) RetireList {
        std::vector<std::unique_ptr<T>> retire_references;
        std::mutex list_mutex;
    };

    struct alignas(THREAD_RECORD_ALIGNMENT) ThreadRecord {
        size_t thread_id;
        size_t reclamation_counter;
    };

    template <typename T>
    class EpochReclamation {
        friend class EpochReclamationTest;

    private:
        std::array<ThreadSlot, MAX_THREADS> thread_slots;
        std::array<RetireList<T>, RETIRE_CLUSTER> retire_lists;

        static thread_local std::unordered_map<const EpochReclamation<T>*, ThreadRecord>
            thread_records;
        std::atomic<size_t> last_reclaimed_epoch{SIZE_MAX};
        std::atomic<size_t> next_thread_id{0};
        std::atomic<size_t> epoch_total{0};

        ThreadRecord& get_or_assign_thread_record() {
            auto iterator = thread_records.find(this);
            if (iterator == thread_records.end()) {
                const size_t new_id = next_thread_id.fetch_add(1, std::memory_order_relaxed);
                if (std::cmp_greater_equal(new_id, MAX_THREADS)) {
                    throw std::runtime_error("Too many threads registered for epoch reclamation");
                }
                iterator =
                    thread_records
                        .emplace(this, ThreadRecord{.thread_id = new_id, .reclamation_counter = 0})
                        .first;
                slot_of(new_id).is_used.store(true, std::memory_order_release);
            }
            return iterator->second;
        }

        [[nodiscard]] ThreadRecord& engaged_thread_record() const noexcept {
            return thread_records.find(this)->second;
        }

        ThreadSlot& slot_of(const size_t thread_id) noexcept {
            return *std::next(thread_slots.begin(), static_cast<ptrdiff_t>(thread_id));
        }

        RetireList<T>& retire_list_of(const size_t epoch) noexcept {
            return *std::next(retire_lists.begin(), static_cast<ptrdiff_t>(epoch % RETIRE_CLUSTER));
        }

        [[nodiscard]] size_t get_oldest_active_epoch() const noexcept {
            size_t min_epoch = epoch_total.load(std::memory_order_acquire);

            for (const auto& slot : thread_slots) {
                if (slot.is_used.load(std::memory_order_acquire)) {
                    const size_t local_epoch = slot.local_epoch.load(std::memory_order_acquire);

                    if (local_epoch != INACTIVE_SIGN) {
                        min_epoch = std::min(local_epoch, min_epoch);
                    }
                }
            }

            return min_epoch;
        }

        void try_advance_epoch() noexcept {
            size_t current_epoch = epoch_total.load(std::memory_order_acquire);
            const size_t min_epoch = this->get_oldest_active_epoch();

            if (min_epoch == current_epoch) {
                epoch_total.compare_exchange_strong(
                    current_epoch,
                    current_epoch + 1,
                    std::memory_order_release,
                    std::memory_order_acquire
                );
            }
        }

        void reclaim_obsolete_memory() {
            const size_t min_epoch = get_oldest_active_epoch();
            if (min_epoch < EPOCH_WINDOW_SIZE) {
                return;
            }

            const size_t safe_epoch = min_epoch - EPOCH_WINDOW_SIZE;
            const size_t last_reclaimed = last_reclaimed_epoch.load(std::memory_order_acquire);
            const size_t start_epoch = (last_reclaimed == SIZE_MAX) ? 0 : last_reclaimed + 1;

            for (size_t epoch = start_epoch; epoch <= safe_epoch; ++epoch) {
                RetireList<T>& list = retire_list_of(epoch);
                const std::scoped_lock list_guard(list.list_mutex);
                list.retire_references.clear();
            }

            if (last_reclaimed == SIZE_MAX || safe_epoch > last_reclaimed) {
                last_reclaimed_epoch.store(safe_epoch, std::memory_order_release);
            }
        }

    public:
        EpochReclamation() = default;
        ~EpochReclamation() = default;

        EpochReclamation(const EpochReclamation&) = delete;
        EpochReclamation& operator=(const EpochReclamation&) = delete;
        EpochReclamation(EpochReclamation&&) = delete;
        EpochReclamation& operator=(EpochReclamation&&) = delete;

        void engage_epoch_user() {
            ThreadSlot& slot = slot_of(this->get_or_assign_thread_record().thread_id);
            slot.is_used.store(true, std::memory_order_release);

            const size_t current_epoch = epoch_total.load(std::memory_order_acquire);
            slot.local_epoch.store(current_epoch, std::memory_order_release);
        }

        void free_epoch_user() noexcept {
            ThreadRecord& record = this->engaged_thread_record();
            slot_of(record.thread_id).local_epoch.store(INACTIVE_SIGN, std::memory_order_release);

            if (++record.reclamation_counter % RECLAMATION_FREQUENCY == 0) {
                this->try_advance_epoch();
                this->reclaim_obsolete_memory();
            }
        }

        void retire_reference(gsl::owner<T*> reference) {
            std::unique_ptr<T> retired(reference);
            RetireList<T>& list = retire_list_of(epoch_total.load(std::memory_order_acquire));

            const std::scoped_lock list_guard(list.list_mutex);
            list.retire_references.push_back(std::move(retired));
        }
    };

    template <typename T>
    thread_local std::unordered_map<const EpochReclamation<T>*, ThreadRecord>
        EpochReclamation<T>::thread_records;
}  // namespace multithreading::utilities::performance
