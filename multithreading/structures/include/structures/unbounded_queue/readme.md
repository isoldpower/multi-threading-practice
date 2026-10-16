# Multithreading Unbounded Queue

## Table of Contents

1. [Overview](#overview)
2. **API Reference**
    - 2.1 [UnboundedQueue\<T\> Interface](#unboundedqueuet-interface)
    - 2.2 [Fine-Grained Lock Unbounded Queue](#fine-grained-lock-unbounded-queue)
    - 2.3 [Lock-Free Unbounded Queue](#lock-free-unbounded-queue)

---

## `UnboundedQueue<T>` Interface

### Overview

`UnboundedQueue<T>` is the abstract base interface for all unbounded queue implementations in the `multithreading::structures::unbounded_queue` namespace. It defines a uniform API that concrete implementations — such as `FGLockUnboundedQueue` and `LockFreeUnboundedQueue` — must fulfill, allowing them to be used interchangeably regardless of their internal synchronization strategy.

The interface is non-copyable and non-movable by design, as concurrent data structures carry internal state (mutexes, atomic pointers, semaphores) that cannot be safely transferred or duplicated.

---

### Interface Contract

#### Insertion

| Method                    | Description                                             |
|---------------------------|---------------------------------------------------------|
| `enqueue(const T& value)` | Copies `value` and appends it to the back of the queue. |
| `enqueue(T&& value)`      | Moves `value` and appends it to the back of the queue.  |

#### Removal

All removal operations return `std::optional<T>` — the value of the dequeued element on success, or `std::nullopt` if the queue is empty or the timeout expires. The caller does not need to manage node memory.

| Method                                 | Description                                                                                                 |
|----------------------------------------|-------------------------------------------------------------------------------------------------------------|
| `try_dequeue()`                        | Attempts to dequeue and return the front element immediately. Returns `std::nullopt` if the queue is empty. |
| `wait_dequeue(duration timeout)`       | Blocks until an element is available or the timeout expires, then dequeues and returns it.                  |
| `wait_dequeue_async(duration timeout)` | Same as `wait_dequeue` but executes asynchronously, returning a `std::future<std::optional<T>>`.            |

#### Inspection

| Method       | Description                                       |
|--------------|---------------------------------------------------|
| `is_empty()` | Returns `true` if the queue contains no elements. |

---

### Concurrency Semantics

The interface itself makes no concurrency guarantees — those are entirely the responsibility of each concrete implementation. Callers should consult the specific implementation's documentation to understand its thread-safety model, progress guarantees, and any caveats around blocking behavior and timeout accuracy.

---

### Implementation Notes for Subclasses

- The destructor is `virtual`, ensuring correct cleanup when deleting through a base pointer.
- `wait_dequeue_async` is provided as a convenience wrapper. Implementations are responsible for deciding how the async work is scheduled — the default pattern of `std::async(std::launch::async, ...)` spawns a new thread per call, which may not be appropriate under high call rates.
- `is_empty()` is not required to be strongly consistent under concurrent modification — implementations may return a value that is transiently stale. Document any such weakening explicitly.

---

## Fine-Grained Lock Unbounded Queue

### Overview

`FGLockUnboundedQueue<T>` is a thread-safe unbounded FIFO queue using **fine-grained locking**. Rather than protecting the entire structure with a single mutex, it uses two separate mutexes — one for the head and one for the tail — allowing enqueue and dequeue operations to proceed **concurrently** without blocking each other in the common case.

The list uses a **sentinel (dummy) head node** that is never removed, which simplifies edge case handling and is the key structural property that makes two-lock concurrency correct.

---

### Algorithm — Two-Lock Queue

The queue is implemented as a singly-linked list with the sentinel node always sitting at `head`. This separation is what allows `head_mu` and `tail_mu` to guard truly independent parts of the structure:

- **Enqueue** acquires only `tail_mu`, links the new node after the current tail, and advances the `tail` pointer.
- **Dequeue** acquires only `head_mu`, promotes the first real node to become the new dummy, extracts its value, and frees the old sentinel.

Because the sentinel ensures `head` and `tail` never refer to the same meaningful data node during normal operation, the two locks protect non-overlapping regions of the list. The only time both locks are needed simultaneously is in the destructor during teardown.

`wait_dequeue` blocks the calling thread on a `std::condition_variable` associated with `head_mu`, waking when a new item is enqueued or the timeout expires.

---

### Complexity

| Operation      | Time Complexity | Notes                                 |
|----------------|-----------------|---------------------------------------|
| `enqueue`      | O(1)            | Acquires `tail_mu` only               |
| `try_dequeue`  | O(1)            | Acquires `head_mu` only               |
| `wait_dequeue` | O(1)            | Blocks on `head_mu`; woken by enqueue |
| `is_empty`     | O(1)            | Acquires `head_mu` only               |
| Destructor     | O(n)            | Acquires both locks                   |

Enqueue and dequeue **do not contend with each other**, only with operations of the same kind. This gives meaningful throughput improvements over a single-lock design under concurrent producer/consumer workloads.

---

### Pitfalls and Limitations

**The single-element edge case is subtle.** When the queue has exactly one element, `head->next()` and `tail` point to the same node. A concurrent enqueue (holding `tail_mu`) and dequeue (holding `head_mu`) operating on that node simultaneously is still safe because they access different fields — enqueue writes `tail->next` while dequeue reads `head->next` — but this is a non-obvious invariant that must be preserved if the node structure is ever modified.

**`wait_dequeue` timeout is measured from lock acquisition, not call entry.** The `condition_variable::wait_for` duration begins once `head_mu` is acquired. Any time spent waiting to acquire the lock is not accounted for, so the effective timeout seen by the caller may be slightly shorter than requested.

**`wait_dequeue_async` spawns a real thread per call.** Each invocation launches a new `std::async` thread that blocks internally on `head_mu`. Under high call rates this can lead to thread explosion and lock contention. A dedicated consumer thread or a thread pool should be preferred in throughput-sensitive scenarios.

**Dynamic allocation per enqueue.** Every enqueue allocates a `FGNode<T>` on the heap. Under high-throughput scenarios this can become a bottleneck due to allocator contention. A node pool or slab allocator would be a natural optimization.

**No copy or move semantics.** The queue is non-copyable and non-movable by design, as safely transferring live mutexes and raw node pointers is non-trivial.

---

## Lock-Free Unbounded Queue

### Overview

`LockFreeUnboundedQueue<T>` is a thread-safe unbounded FIFO queue that avoids mutexes entirely, relying instead on **atomic CAS (Compare-And-Swap) operations** for synchronization. It is based on the **Michael-Scott queue algorithm** and uses a **sentinel (dummy) head node** identical in purpose to the fine-grained lock version.

The implementation is configurable via `LockFreeQueueConfig`, which allows tuning the maximum number of CAS retry attempts (`maxUpdateDepth`) before the algorithm gives up and throws. The default is `100` attempts.

---

### Algorithm — Michael-Scott Queue with Cooperative Tail Advancement

The queue is a singly-linked list where both `head` and `tail` are `cache-line aligned` atomic pointers (`AlignedField`), preventing false sharing between producers and consumers. All operations follow a **CAS retry loop** bounded by `maxAlgorithmDepth`.

**Enqueue** works in two steps:
1. CAS `last->next` from `nullptr` to the new node, linking it into the list.
2. CAS `tail` forward from `last` to the new node. This second step is **best-effort** — if it fails, another thread will complete it cooperatively.

**Dequeue** reads `head->next()` (the first real node), copies its value, then CAS-swaps `head` forward to that node, freeing the old sentinel. If `head == tail` but `head->next() != nullptr`, the tail is lagging — the dequeuing thread advances it before retrying.

**Cooperative tail advancement:** Both enqueue and dequeue detect a stale `tail` (i.e. `tail->next != nullptr`) and attempt to advance it on behalf of the thread that performed the enqueue but hasn't yet updated `tail`. This prevents the tail from falling arbitrarily far behind under concurrent enqueues.

`wait_dequeue` uses a `std::counting_semaphore` rather than a condition variable. Each enqueue releases the semaphore by one; `wait_dequeue` acquires it with `try_acquire_for`, then calls `try_dequeue`. Because `try_dequeue` callers can race with `wait_dequeue` for the same item, a successful semaphore acquire does not guarantee a successful dequeue — the loop retries until the deadline is reached, with a final `try_dequeue` attempt after expiry.

---

### Complexity

| Operation      | Time Complexity | Notes                                                           |
|----------------|-----------------|-----------------------------------------------------------------|
| `enqueue`      | O(1) amortized  | Retries on CAS failure; bounded by `maxAlgorithmDepth`          |
| `try_dequeue`  | O(1) amortized  | Retries on CAS failure; bounded by `maxAlgorithmDepth`          |
| `wait_dequeue` | O(1) amortized  | Semaphore-based; may retry on race with `try_dequeue` callers   |
| `is_empty`     | O(1) amortized  | Consistency-checked atomic read; bounded by `maxAlgorithmDepth` |
| Destructor     | O(n)            | Drains via `try_dequeue` loop                                   |

"Amortized" accounts for CAS retry loops under contention. In the worst case a thread can be repeatedly preempted and retry up to `maxAlgorithmDepth` times before throwing.

---

### Pitfalls and Limitations

**`maxAlgorithmDepth` exhaustion throws.** Unlike most concurrent structures that spin indefinitely, this implementation throws `std::runtime_error` if the retry limit is exceeded in `enqueue_node`, `dequeue_node`, or `is_empty`. This is a deliberate safety valve against livelock under extreme contention, but callers must be prepared to catch it. The default of 100 attempts is generous for most workloads, but should be increased for very high thread counts.

**`wait_dequeue` can race with `try_dequeue`.** The semaphore count and the actual queue contents are not updated atomically. A `try_dequeue` call concurrent with `wait_dequeue` may consume the item the semaphore signaled for, causing `wait_dequeue` to loop and re-check the deadline. In the worst case, repeated `try_dequeue` calls can starve a waiting `wait_dequeue` caller until its deadline expires.

**Tail advancement is best-effort on enqueue.** After a successful `last->next` CAS, the tail CAS is attempted once and not retried — the method returns regardless of whether it succeeds. This is correct because the next enqueue or dequeue will cooperatively advance the tail, but it means `tail` can transiently lag by one node.

**`is_empty` is a snapshot, not a strong guarantee.** The consistency check (re-reading `head` after loading `tail`) reduces the window for stale reads but does not eliminate it. A result of `true` does not guarantee the queue will still be empty by the time the caller acts on it.

**`wait_dequeue_async` spawns a real thread per call.** Same caveat as the fine-grained lock version — each call to `wait_dequeue_async` launches a new `std::async` thread. Prefer a dedicated consumer thread under high call rates.

**Dynamic allocation per enqueue.** Every enqueue allocates a `LockFreeNode<T>` on the heap. Node deallocation happens inline in `dequeue_node` immediately after a successful `head` CAS, without a deferred reclamation scheme. This is safe here because the sentinel pattern ensures no other thread holds a reference to the freed node at that point, but it does mean allocator contention under high throughput is possible. Unlike the lock-free linked list, **no epoch-based reclamation is used** — correctness relies on the sentinel-based ownership transfer being sufficient.

**No copy or move semantics.** The queue is non-copyable and non-movable by design.