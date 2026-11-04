#pragma once

#include <utilities/performance/AlignedField.h>
#include <utilities/performance/Owner.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace multithreading::utilities::performance {

    constexpr size_t EPOCH_WINDOW_SIZE = 2;
    constexpr size_t MAX_THREADS = 128;
    constexpr size_t INACTIVE_SIGN = static_cast<size_t>(-1);
    constexpr size_t RECLAMATION_FREQUENCY = 100;

    /** @brief The epoch one thread is reading under, alone on its cache line: its owner writes
     * it on every operation, and sharing the line would make neighbours invalidate each other. */
    // The padding up to a whole line is the point, so the packing check does not apply
    struct alignas(CACHE_LINE_SIZE) ThreadSlot {  // NOLINT(altera-struct-pack-align)
        std::atomic<size_t> local_epoch{INACTIVE_SIGN};
        std::atomic<bool> is_used{false};
    };

    /** @brief Nodes one thread has retired, in the order of the epochs they were retired in.
     * Only its owner appends, so the lock is almost never contended; other threads take it
     * only to reclaim, and never wait for it. */
    template <typename T>
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct alignas(CACHE_LINE_SIZE) RetireList {
        struct Retired {  // NOLINT(altera-struct-pack-align)
            size_t epoch;
            std::unique_ptr<T> reference;
        };

        std::mutex list_mutex;
        std::vector<Retired> retired;
    };

    struct ThreadRecord {  // NOLINT(altera-struct-pack-align)
        size_t thread_id;
        size_t reclamation_counter;
        size_t guard_depth;
    };

    template <typename T>
    class EpochReclamation {
        friend class EpochReclamationTest;

    private:
        /** One thread's view of the reclamations it has used, keyed by instance id rather than
         * address, so a reclamation built where a destroyed one lived never inherits its ids.
         * The last one used is cached, as a thread nearly always keeps to one structure. */
        struct ThreadRecords {  // NOLINT(altera-struct-pack-align)
            // A private bag of per-thread data; only EpochReclamation ever sees it
            // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
            std::uint64_t cached_instance{0};
            ThreadRecord* cached_record{nullptr};
            std::unordered_map<std::uint64_t, ThreadRecord> records;
            // NOLINTEND(misc-non-private-member-variables-in-classes)

            ThreadRecords() = default;
            ThreadRecords(const ThreadRecords&) = delete;
            ThreadRecords& operator=(const ThreadRecords&) = delete;
            ThreadRecords(ThreadRecords&&) = delete;
            ThreadRecords& operator=(ThreadRecords&&) = delete;

            /** Runs as the thread exits: hands its slots back to every reclamation still alive,
             * so a structure can outlive any number of threads, not just MAX_THREADS of them. */
            ~ThreadRecords() {
                if (records.empty()) {
                    return;
                }

                LiveInstances& live = live_instances();
                const std::scoped_lock lock(live.instances_mutex);
                for (const auto& [instance, record] : records) {
                    if (const auto found = live.instances.find(instance);
                        found != live.instances.end()) {
                        found->second->release_slot(record.thread_id);
                    }
                }
            }
        };

        /** Every reclamation alive, so an exiting thread can tell which of the ones it used it
         * may still touch. */
        struct LiveInstances {  // NOLINT(altera-struct-pack-align)
            std::mutex instances_mutex;
            std::unordered_map<std::uint64_t, EpochReclamation*> instances;
        };

        /** Never destroyed: threads exit, and reclamations are destroyed, in no order relative
         * to static destruction, and both still need it then. */
        static LiveInstances& live_instances() {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            static auto* const live = new LiveInstances();
            return *live;
        }

        // Ids start at 1, so the zeroed cache never matches a real instance
        static inline std::atomic<std::uint64_t> next_instance_id{1};
        static thread_local ThreadRecords thread_records;

        std::array<ThreadSlot, MAX_THREADS> thread_slots;
        std::array<RetireList<T>, MAX_THREADS> retire_lists;
        const std::uint64_t instance_id{next_instance_id.fetch_add(1, std::memory_order_relaxed)};

        std::atomic<size_t> slots_high_water{0};
        std::atomic<size_t> epoch_total{0};

        ThreadRecord& get_or_assign_thread_record() {
            ThreadRecords& local = thread_records;
            if (local.cached_instance == instance_id) {
                return *local.cached_record;
            }

            auto iterator = local.records.find(instance_id);
            if (iterator == local.records.end()) {
                const size_t new_id = claim_slot();
                iterator = local.records
                               .emplace(
                                   instance_id,
                                   ThreadRecord{
                                       .thread_id = new_id,
                                       .reclamation_counter = 0,
                                       .guard_depth = 0
                                   }
                               )
                               .first;
            }

            // Map nodes never move, so the pointer stays valid as other entries are added
            local.cached_instance = instance_id;
            local.cached_record = &iterator->second;
            return iterator->second;
        }

        /** Takes the first free slot, raising the high-water mark scans stop at.
         * @throws std::runtime_error when MAX_THREADS threads hold slots at once. */
        size_t claim_slot() {
            for (size_t thread_id = 0; thread_id < MAX_THREADS; ++thread_id) {
                bool expected = false;
                if (slot_of(thread_id).is_used.compare_exchange_strong(
                        expected,
                        true,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed
                    )) {
                    size_t seen = slots_high_water.load(std::memory_order_relaxed);
                    while (
                        seen < thread_id + 1 && !slots_high_water.compare_exchange_weak(
                                                    seen,
                                                    thread_id + 1,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed
                                                )
                    ) {
                    }
                    return thread_id;
                }
            }

            throw std::runtime_error("Too many threads registered for epoch reclamation");
        }

        /** Frees an exited thread's slot for the next thread. Its retired nodes stay in the
         * slot's list; any thread reclaims them once they are safe, the slot's next owner
         * included. */
        void release_slot(const size_t thread_id) noexcept {
            ThreadSlot& slot = slot_of(thread_id);
            slot.local_epoch.store(INACTIVE_SIGN, std::memory_order_release);
            slot.is_used.store(false, std::memory_order_release);
        }

        /** The record engage_epoch_user() made sure exists for this thread. */
        [[nodiscard]] ThreadRecord& engaged_thread_record() const noexcept {
            ThreadRecords& local = thread_records;
            if (local.cached_instance == instance_id) {
                return *local.cached_record;
            }
            return local.records.find(instance_id)->second;
        }

        ThreadSlot& slot_of(const size_t thread_id) noexcept {
            return *std::next(thread_slots.begin(), static_cast<ptrdiff_t>(thread_id));
        }

        RetireList<T>& retire_list_of(const size_t thread_id) noexcept {
            return *std::next(retire_lists.begin(), static_cast<ptrdiff_t>(thread_id));
        }

        /** Slots past this were never handed out, so scans can stop here. */
        [[nodiscard]] size_t registered_threads() const noexcept {
            return slots_high_water.load(std::memory_order_acquire);
        }

        [[nodiscard]] size_t get_oldest_active_epoch() const noexcept {
            // Pairs with the fence in engage_epoch_user: either this scan sees a reader's
            // announcement, or that reader sees the structure with the node already unlinked
            std::atomic_thread_fence(std::memory_order_seq_cst);
            size_t min_epoch = epoch_total.load(std::memory_order_acquire);

            for (const ThreadSlot& slot : std::span(thread_slots).first(registered_threads())) {
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
                    std::memory_order_acq_rel,
                    std::memory_order_acquire
                );
            }
        }

        /** Frees every node retired at least EPOCH_WINDOW_SIZE epochs before the oldest one a
         * thread is still reading under. A list another thread is busy with is left for the
         * next pass rather than waited for. */
        void reclaim_obsolete_memory() {
            const size_t min_epoch = get_oldest_active_epoch();
            if (min_epoch < EPOCH_WINDOW_SIZE) {
                return;
            }
            const size_t safe_epoch = min_epoch - EPOCH_WINDOW_SIZE;

            for (RetireList<T>& list : std::span(retire_lists).first(registered_threads())) {
                const std::unique_lock list_guard(list.list_mutex, std::try_to_lock);
                if (!list_guard.owns_lock()) {
                    continue;
                }

                // Epochs only grow, so everything safe to free is a prefix of the list
                auto& retired = list.retired;
                const auto first_unsafe = std::ranges::find_if(
                    retired,
                    [safe_epoch](const size_t epoch) {
                        return epoch > safe_epoch;
                    },
                    &RetireList<T>::Retired::epoch
                );
                retired.erase(retired.begin(), first_unsafe);
            }
        }

    public:
        EpochReclamation() {
            LiveInstances& live = live_instances();
            const std::scoped_lock lock(live.instances_mutex);
            live.instances.emplace(instance_id, this);
        }

        ~EpochReclamation() {
            LiveInstances& live = live_instances();
            const std::scoped_lock lock(live.instances_mutex);
            live.instances.erase(instance_id);
        }

        EpochReclamation(const EpochReclamation&) = delete;
        EpochReclamation& operator=(const EpochReclamation&) = delete;
        EpochReclamation(EpochReclamation&&) = delete;
        EpochReclamation& operator=(EpochReclamation&&) = delete;

        /** @brief Marks the calling thread as reading. Guards may nest; only the outermost one
         * announces, so an inner guard ending never exposes what the outer one still reads. */
        void engage_epoch_user() {
            ThreadRecord& record = this->get_or_assign_thread_record();
            if (record.guard_depth++ > 0) {
                return;
            }

            ThreadSlot& slot = slot_of(record.thread_id);
            size_t announced = epoch_total.load(std::memory_order_acquire);
            while (true) {
                slot.local_epoch.store(announced, std::memory_order_relaxed);
                // The announcement must be visible before this thread reads any shared pointer;
                // a release store alone lets those reads move ahead of it
                std::atomic_thread_fence(std::memory_order_seq_cst);

                const size_t current = epoch_total.load(std::memory_order_acquire);
                if (current == announced) {
                    return;
                }
                // The epoch moved while announcing, so a reclaimer may have missed the old value
                announced = current;
            }
        }

        /** @brief Ends the calling thread's innermost guard; it must have engaged first. */
        void free_epoch_user() noexcept {
            ThreadRecord& record = this->engaged_thread_record();
            if (--record.guard_depth > 0) {
                return;
            }
            slot_of(record.thread_id).local_epoch.store(INACTIVE_SIGN, std::memory_order_release);

            if (++record.reclamation_counter % RECLAMATION_FREQUENCY == 0) {
                this->try_advance_epoch();
                this->reclaim_obsolete_memory();
            }
        }

        void retire_reference(gsl::owner<T*> reference) {
            std::unique_ptr<T> retired(reference);
            const size_t thread_id = this->get_or_assign_thread_record().thread_id;
            const size_t epoch = epoch_total.load(std::memory_order_acquire);

            RetireList<T>& list = retire_list_of(thread_id);
            const std::scoped_lock list_guard(list.list_mutex);
            list.retired.push_back({.epoch = epoch, .reference = std::move(retired)});
        }
    };

    template <typename T>
    thread_local typename EpochReclamation<T>::ThreadRecords EpochReclamation<T>::thread_records;
}  // namespace multithreading::utilities::performance
