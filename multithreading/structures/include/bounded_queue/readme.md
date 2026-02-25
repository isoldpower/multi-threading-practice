# Multithreading Bounded Queue

## Table of Contents

1. [Overview](#overview)
2. **API Reference**
    - 2.1 [BoundedQueue\<T\> Interface](#boundedqueuet-interface)
    - 2.2 [Fine-Grained Lock Bounded Queue](#fine-grained-lock-bounded-queue)
    - 2.3 [Lock-Free Bounded Queue](#lock-free-bounded-queue)

---

## `BoundedQueue<T>` Interface

### Overview

`BoundedQueue<T>` is the abstract base interface for all bounded queue implementations in the `multithreading::structures::bounded_queue` namespace. It defines a uniform API that concrete implementations — such as `FGLockBoundedQueue` and `LockFreeBoundedQueue` — must fulfill, allowing them to be used interchangeably regardless of their internal synchronization strategy.

Unlike `UnboundedQueue<T>`, this interface models a capacity-constrained queue where insertion can fail. Enqueue operations are therefore non-blocking and return a `bool` to indicate success or failure, placing backpressure handling in the caller's domain.

The interface is non-copyable and non-movable by design, as concurrent data structures carry internal state that cannot be safely transferred or duplicated.

---

### Interface Contract

#### Insertion

Enqueue operations return `false` immediately if the queue is at capacity or is shutting down, rather than blocking. The caller is responsible for retry logic or backpressure handling.

| Method                        | Description                                                                                         |
|-------------------------------|-----------------------------------------------------------------------------------------------------|
| `try_enqueue(const T& value)` | Copies `value` and appends it to the back. Returns `true` on success, `false` if the queue is full. |
| `try_enqueue(T&& value)`      | Moves `value` and appends it to the back. Returns `true` on success, `false` if the queue is full.  |

#### Removal

All removal operations return `std::optional<T>` — the value of the dequeued element on success, or `std::nullopt` if the queue is empty or the timeout expires.

| Method                                 | Description                                                                                                 |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------|
| `try_dequeue()`                        | Attempts to dequeue and return the front element immediately. Returns `std::nullopt` if the queue is empty. |
| `wait_dequeue(duration timeout)`       | Blocks until an element is available or the timeout expires, then dequeues and returns it.                  |
| `wait_dequeue_async(duration timeout)` | Same as `wait_dequeue` but executes asynchronously, returning a `std::future<std::optional<T>>`.            |

#### Inspection

Both inspection methods accept an `isPrecise` flag. When `false`, implementations may return a result based on an atomic counter or snapshot that is fast but transiently stale. When `true`, implementations acquire the appropriate lock to provide a strongly consistent answer. Callers that need accuracy (e.g. for flow control decisions) should use `isPrecise = true`; callers in hot paths that only need a hint should use `isPrecise = false`.

| Method                     | Description                                           |
|----------------------------|-------------------------------------------------------|
| `is_empty(bool isPrecise)` | Returns `true` if the queue contains no elements.     |
| `is_full(bool isPrecise)`  | Returns `true` if the queue has reached its capacity. |

---

### Concurrency Semantics

The interface itself makes no concurrency guarantees — those are entirely the responsibility of each concrete implementation. Callers should consult the specific implementation's documentation to understand its thread-safety model, progress guarantees, and shutdown behavior.

---

### Implementation Notes for Subclasses

- The destructor is `virtual`, ensuring correct cleanup when deleting through a base pointer.
- Implementations should document what happens to in-flight operations when the queue is destroyed — particularly whether blocked `wait_dequeue` callers are unblocked and what value they observe.
- `wait_dequeue_async` is provided as a convenience wrapper. Implementations are responsible for scheduling — the default pattern of `std::async(std::launch::async, ...)` spawns a new thread per call, which may not be appropriate under high call rates.
- The `isPrecise` distinction on `is_empty` and `is_full` is an interface-level contract. Implementations that have no meaningful difference between precise and imprecise reads may ignore the flag, but should document this.

---

## Fine-Grained Lock Bounded Queue

### Overview

`FGLockBoundedQueue<T>` is a thread-safe bounded FIFO queue using **fine-grained locking**. Like its unbounded counterpart, it uses two separate mutexes — one for the head and one for the tail — allowing concurrent enqueue and dequeue operations to proceed without blocking each other in the common case.

The queue uses a **sentinel (dummy) head node**, an atomic `size_counter`, and an `is_shutdown` flag to coordinate capacity enforcement and graceful destruction.

---

### Algorithm — Two-Lock Queue with Capacity Enforcement

The structure mirrors the unbounded fine-grained lock queue, with the addition of capacity tracking and a shutdown mechanism:

- **Enqueue** (`try_enqueue`) acquires `tail_mu`, then performs an atomic `fetch_add` on `size_counter` to speculatively claim a slot. If the resulting count exceeds `size_limit`, it rolls back with a `fetch_sub` and returns `false`. On success it links the node and notifies `enqueue_condition`.
- **Dequeue** (`try_dequeue`) first performs a relaxed load on `size_counter` as a fast-path guard to avoid locking when the queue is obviously empty. It then acquires `head_mu`, re-validates the state, and performs the sentinel-based dequeue identical to the unbounded version.
- **Shutdown** is managed via the `is_shutdown` atomic flag, set in the destructor before acquiring both locks. Any `wait_dequeue` caller blocked on `enqueue_condition` will wake up, observe `is_shutdown`, and return `std::nullopt`.

Both `is_empty` and `is_full` have two variants: a fast relaxed atomic read (`isPrecise = false`) and a mutex-guarded read (`isPrecise = true`).

---

### Complexity

| Operation              | Time Complexity | Notes                                                         |
|------------------------|-----------------|---------------------------------------------------------------|
| `try_enqueue`          | O(1)            | Acquires `tail_mu`; rolls back on capacity exceeded           |
| `try_dequeue`          | O(1)            | Relaxed counter check, then acquires `head_mu`                |
| `wait_dequeue`         | O(1)            | Blocks on `head_mu`; woken by enqueue or shutdown             |
| `is_empty` / `is_full` | O(1)            | Relaxed atomic read or mutex-guarded depending on `isPrecise` |
| Destructor             | O(n)            | Acquires both locks, drains all nodes                         |

Enqueue and dequeue do not contend with each other in the common case, matching the throughput characteristics of the unbounded two-lock queue.

---

### Pitfalls and Limitations

**`size_counter` and structural state are not updated atomically.** The counter is incremented speculatively before the node is linked, and decremented after the node is removed. This means a `is_full()` check with `isPrecise = false` may return `true` transiently even when a slot is about to become available, and `is_empty()` may return `false` briefly after the last item has been dequeued but before the counter is decremented.

**`try_enqueue` allocates before checking capacity under the lock.** The `FGNode<T>` is heap-allocated by the caller (`FGLockBoundedQueue`) before `try_enqueue` is called on the impl. If the queue turns out to be full, the node is immediately deleted. This introduces a redundant allocation/deallocation cycle on every rejected enqueue under contention.

**Shutdown wakes all `wait_dequeue` waiters.** The destructor calls `notify_all()` on `enqueue_condition` before acquiring both locks, so all blocked `wait_dequeue` callers are unblocked and will return `std::nullopt`. Callers must handle this gracefully if destruction can race with active consumers.

**`wait_dequeue` timeout is measured from lock acquisition.** The same caveat as the unbounded version applies — time spent waiting to acquire `head_mu` is not counted against the timeout.

**`wait_dequeue_async` spawns a real thread per call.** Each invocation launches a new `std::async` thread. Prefer a dedicated consumer thread under high call rates.

**No copy or move semantics.** The queue is non-copyable and non-movable by design.

---

## Lock-Free Bounded Queue

### Overview

`LockFreeBoundedQueue<T>` is a thread-safe bounded FIFO queue that avoids mutexes entirely, using a **circular buffer of atomic sequence slots** and **CAS operations** for synchronization. It is based on the classic **Dmitry Vyukov MPMC (Multi-Producer Multi-Consumer) queue** algorithm.

Rather than managing a linked list, it operates on a fixed-size pre-allocated array of `CircularSlot<T>` entries, each carrying an atomic sequence number that encodes whether the slot is empty or full. Memory for values is managed via `SlotValue<T>`, a manual aligned storage wrapper that constructs and destructs `T` in-place without requiring `T` to be default-constructible at all times.

---

### Algorithm — Circular Buffer with Sequence Numbers

The buffer has a fixed `capacity`. Two independent atomic counters drive the queue:

- `enqueue_pos` — the next position a producer will attempt to claim.
- `dequeue_pos` — the next position a consumer will attempt to claim.

Each slot holds an atomic `sequence` number that encodes its state relative to the current lap:

- `sequence == position` → slot is **empty** and ready to be written (lap-aligned empty state).
- `sequence == position + 1` → slot is **full** and ready to be read.
- `sequence < position` → slot is still occupied from the previous lap — the queue is **full**.
- `sequence > position` → another thread has already claimed this slot — **retry**.

**Enqueue** loads `enqueue_pos`, computes `lap_position = position % capacity`, and reads the slot's sequence. If the sequence equals `position` (empty), it CAS-advances `enqueue_pos` to claim the slot, emplaces the value, and stores `position + 1` as the new sequence to signal "full". If the sequence is less than `position`, the queue is full and the method returns `false`.

**Dequeue** mirrors this: it loads `dequeue_pos`, reads the slot's sequence, and proceeds only when `sequence == position + 1` (full). After a successful CAS on `dequeue_pos`, it takes the value and stores `position + capacity` as the new sequence, advancing the slot into the next lap's empty state.

`wait_dequeue` uses a `std::counting_semaphore` — each successful enqueue releases one permit. The same race between `wait_dequeue` and concurrent `try_dequeue` callers as in the lock-free unbounded queue applies here.

---

### Complexity

| Operation              | Time Complexity | Notes                                                         |
|------------------------|-----------------|---------------------------------------------------------------|
| `try_enqueue`          | O(1) amortized  | CAS retry loop under contention                               |
| `try_dequeue`          | O(1) amortized  | CAS retry loop under contention                               |
| `wait_dequeue`         | O(1) amortized  | Semaphore-based; may retry on race with `try_dequeue` callers |
| `is_empty` / `is_full` | O(1) amortized  | Consistency-checked atomic read loop                          |
| Destructor             | O(n)            | Drains via `try_dequeue` loop                                 |

"Amortized" accounts for CAS retry loops under contention. Unlike the lock-free unbounded queue, there is no `maxAlgorithmDepth` cap — the loops spin until they succeed or determine the queue is full/empty. Under pathological contention this can spin indefinitely.

---

### Pitfalls and Limitations

**No retry bound — spinning is unbounded.** Unlike the lock-free unbounded queue, `try_enqueue` and `try_dequeue` do not have a depth limit and will spin until they either succeed or determine the queue is definitively full or empty. A thread under continuous contention is not guaranteed to make progress in a bounded number of steps, making this obstruction-free but not lock-free in the strict sense.

**`SlotValue<T>` uses manual lifetime management.** `emplace` placement-new constructs a `T` into aligned storage, and `take`/`erase` manually calls the destructor. If the destructor throws or the queue is destroyed with items still in it, the destructor drains via `try_dequeue` to properly destruct each `T`. Callers with non-trivially-destructible types should be aware that destruction happens during `try_dequeue` in the destructor's drain loop.

**`wait_dequeue` can race with `try_dequeue`.** The semaphore count and the actual slot state are not updated atomically. A concurrent `try_dequeue` can consume the item that triggered the semaphore release, causing `wait_dequeue` to spin and retry until its deadline.

**`is_empty` and `is_full` ignore the `isPrecise` flag.** Both `LockFreeBoundedQueue::is_empty` and `is_full` forward to the impl regardless of the `isPrecise` argument, since there is no lock to acquire for a more precise read. The results are based on a consistency-checked atomic snapshot and are still transiently stale.

**`wait_dequeue_async` spawns a real thread per call.** Same caveat as all other implementations — prefer a dedicated consumer thread under high call rates.

**No copy or move semantics.** The queue is non-copyable and non-movable by design.