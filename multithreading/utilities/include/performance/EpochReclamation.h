#pragma once

#include <algorithm>
#include <mutex>
#include <thread>
#include <utility>

namespace multithreading::utilities::performance {

    constexpr size_t EPOCH_WINDOW_SIZE = 2;
    constexpr size_t MAX_THREADS = 128;
    constexpr size_t INACTIVE_SIGN = static_cast<size_t>(-1);
    constexpr size_t RETIRE_CLUSTER = 8;
    constexpr size_t RECLAMATION_FREQUENCY = 100;

    struct alignas(16) ThreadSlot {
        std::atomic<size_t> local_epoch{ INACTIVE_SIGN };
        std::atomic<bool> is_used{ false };
    };

    template <typename T>
    struct RetireList {
        std::vector<T*> retire_references;
        std::mutex list_mutex;
    };

    template <typename T>
    class EpochReclamation {
        friend class EpochReclamationTest;
    private:
        std::array<ThreadSlot, MAX_THREADS> thread_slots;
        std::array<RetireList<T>, RETIRE_CLUSTER> retire_lists;

        static thread_local std::unordered_map<const EpochReclamation<T>*, size_t> instance_thread_ids;
        static thread_local std::unordered_map<const EpochReclamation<T>*, size_t> instance_reclamation_counters;
        std::atomic<size_t> last_reclaimed_epoch{ SIZE_MAX };
        std::atomic<size_t> next_thread_id{0};
        std::atomic<size_t> epoch_total{0};

        size_t get_or_assign_thread_id() {
            auto iterator = instance_thread_ids.find(this);
            if (iterator == instance_thread_ids.end()) {
                size_t new_id = next_thread_id.fetch_add(1, std::memory_order_relaxed);
                if (std::cmp_greater_equal(new_id, MAX_THREADS)) {
                    throw std::runtime_error("Too many threads registered for epoch reclamation");
                }
                instance_thread_ids[this] = new_id;
                thread_slots.at(new_id).is_used.store(true, std::memory_order_release);
                return new_id;
            }
            return iterator->second;
        }

        size_t get_oldest_active_epoch() const {
            size_t min_epoch = epoch_total.load(std::memory_order_acquire);

            for (const auto& slot : thread_slots) {
                if (slot.is_used.load(std::memory_order_acquire)) {
                    size_t const local_epoch = slot.local_epoch.load(std::memory_order_acquire);

                    if (local_epoch != INACTIVE_SIGN) {
                        min_epoch = std::min(local_epoch, min_epoch);
                    }
                }
            }

            return min_epoch;
        }

        void try_advance_epoch() {
            size_t current_epoch = epoch_total.load(std::memory_order_acquire);
            size_t const min_epoch = this->get_oldest_active_epoch();

            if (min_epoch == current_epoch) {
                epoch_total.compare_exchange_strong(
                    current_epoch,
                    current_epoch + 1,
                    std::memory_order_release,
                    std::memory_order_acquire);
            }
        }

        void reclaim_obsolete_memory() {
            const size_t min_epoch = get_oldest_active_epoch();
            if (min_epoch < EPOCH_WINDOW_SIZE) return;

            const size_t safe_epoch = min_epoch - EPOCH_WINDOW_SIZE;
            const size_t last_reclaimed = last_reclaimed_epoch.load(std::memory_order_acquire);
            const size_t start_epoch = (last_reclaimed == SIZE_MAX) ? 0 : last_reclaimed + 1; // ← fix

            for (size_t epoch = start_epoch; epoch <= safe_epoch; ++epoch) {
                RetireList<T>& list = retire_lists[epoch % RETIRE_CLUSTER];
                std::lock_guard list_guard(list.list_mutex);
                for (T* ref : list.retire_references) delete ref;
                list.retire_references.clear();
            }

            if (last_reclaimed == SIZE_MAX || safe_epoch > last_reclaimed) {
                last_reclaimed_epoch.store(safe_epoch, std::memory_order_release);
            }
        }
    public:
        EpochReclamation() = default;
        ~EpochReclamation() {
            for (auto& retire_list : retire_lists) {
                std::lock_guard lock_guard(retire_list.list_mutex);
                for (T* reference : retire_list.retire_references) {
                    delete reference;
                }

                retire_list.retire_references.clear();
            }
        }

        void engage_epoch_user() {
            size_t const thread_id = this->get_or_assign_thread_id();
            thread_slots.at(thread_id).is_used.store(true, std::memory_order_release);

            const size_t current_epoch = epoch_total.load(std::memory_order_acquire);
            thread_slots.at(thread_id).local_epoch.store(current_epoch, std::memory_order_release);
        }

        void free_epoch_user() {
            size_t const thread_id = this->get_or_assign_thread_id();
            thread_slots.at(thread_id).local_epoch.store(INACTIVE_SIGN, std::memory_order_release);

            size_t& counter = instance_reclamation_counters[this];
            if (++counter % RECLAMATION_FREQUENCY == 0) {
                this->try_advance_epoch();
                this->reclaim_obsolete_memory();
            }
        }

        void retire_reference(T* reference) {
            size_t const current_epoch = epoch_total.load(std::memory_order_acquire);
            size_t retire_list_index = current_epoch % RETIRE_CLUSTER;
            RetireList<T>& list = retire_lists[retire_list_index];

            std::lock_guard list_guard(list.list_mutex);
            list.retire_references.push_back(reference);
        }
    };

    template <typename T>
    thread_local std::unordered_map<const EpochReclamation<T>*, size_t>
        EpochReclamation<T>::instance_thread_ids;

    template <typename T>
    thread_local std::unordered_map<const EpochReclamation<T>*, size_t>
        EpochReclamation<T>::instance_reclamation_counters;
} // namespace multithreading::utilities::performance

