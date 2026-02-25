# Multithreading Linked List

## Table of Contents

1. [Overview](#overview)
2. **API Reference**
    - 2.1 [LinkedList<T> Interface](#linkedlistt-interface)
    - 2.2 [Fine-Grained Lock Linked List](#fine-grained-lock-linked-list)
    - 2.3 [Lock-Free Linked List](#lock-free-linked-list)

---

## `LinkedList<T>` Interface

### Overview

`LinkedList<T>` is the abstract base interface for all linked list implementations in the `multithreading::structures::linked_list` namespace. It defines a uniform API that concrete implementations — such as `FGLockLinkedList` and `LockFreeLinkedList` — must fulfill, allowing them to be used interchangeably regardless of their internal synchronization strategy.

The interface is non-copyable and non-movable by design, as concurrent data structures carry internal state (mutexes, atomic pointers, epoch guards) that cannot be safely transferred or duplicated.

---

### Interface Contract

All index-based operations treat the list as **0-indexed**, where index `0` refers to the first real element (i.e. the node immediately after the sentinel head, if one is used by the implementation).

#### Insertion

| Method                          | Description                                                                                               |
|---------------------------------|-----------------------------------------------------------------------------------------------------------|
| `push_front(T item)`            | Inserts `item` at the front of the list.                                                                  |
| `push_back(T item)`             | Appends `item` at the back of the list.                                                                   |
| `push_at(T item, size_t index)` | Inserts `item` after the node at `index`. Returns `true` on success, `false` if `index` is out of bounds. |

#### Removal

All removal operations return `std::optional<T>` — the value of the removed node on success, or `std::nullopt` if the list is empty or the index is out of bounds. The caller does not need to manage node memory.

| Method                 | Description                                 |
|------------------------|---------------------------------------------|
| `pop_front()`          | Removes and returns the first element.      |
| `pop_back()`           | Removes and returns the last element.       |
| `pop_at(size_t index)` | Removes and returns the element at `index`. |

#### Inspection

| Method              | Description                                                |
|---------------------|------------------------------------------------------------|
| `empty()`           | Returns `true` if the list contains no elements.           |
| `size()`            | Returns the current number of elements.                    |
| `contains(T value)` | Returns `true` if any node holds a value equal to `value`. |

---

### Concurrency Semantics

The interface itself makes no concurrency guarantees — those are entirely the responsibility of each concrete implementation. Callers should consult the specific implementation's documentation to understand its thread-safety model, progress guarantees, and any caveats around methods like `size()` or `find()` under concurrent access.

---

### Implementation Notes for Subclasses

- The destructor is `virtual`, ensuring correct cleanup when deleting through a base pointer.
- `push_at` uses a 0-based index relative to the first real element. Implementations using a sentinel head must account for this offset internally and not expose the sentinel to the caller.
- `size()` is not required to be strongly consistent under concurrent modification — implementations may return a value that is transiently stale. Document any such weakening explicitly.


## Lock-Free Linked List

### Overview

`LockFreeLinkedList<T>` is a thread-safe singly-linked list that avoids mutexes entirely, relying instead on **atomic CAS (Compare-And-Swap) operations** and **pointer marking** for synchronization. It also uses a **sentinel head node** identical in purpose to the fine-grained lock version.

Memory reclamation is handled by an **epoch-based reclamation** scheme (`EpochReclamation` / `EpochGuard`), which defers freeing retired nodes until no thread can possibly hold a reference to them — solving the classic ABA and use-after-free problems inherent to lock-free structures.

---

### Algorithm — Two-Phase Deletion with Pointer Marking

The core insight is that deletion cannot be done atomically in a single step on a singly-linked list. Instead, every removal is a two-phase process based on the Harris linked list algorithm:

**Phase 1 — Logical deletion:** The node's `next` pointer is atomically marked by setting its lowest bit (`MARK_BIT = 0x1`). This signals to all other threads that this node is "dead" and should be skipped. The node is still physically in the list.

**Phase 2 — Physical deletion:** The predecessor's `next` pointer is CAS-swapped to bypass the marked node, unlinking it from the list. The node is then retired to the epoch reclamation system.

This relies on the fact that all nodes are at minimum 8-byte aligned (`alignas(8)`), guaranteeing the lowest bit of any valid pointer is always `0` and is therefore safe to use as a mark bit.

---

### Cooperative Helping

When any thread encounters a logically deleted (marked) node during traversal, it does not simply skip it — it calls `try_help_advance()` to attempt the physical deletion itself. This **cooperative helping** pattern prevents marked nodes from accumulating and ensures the list makes global progress even if the thread that performed the logical deletion is delayed or preempted.

`push_back` applies the same principle for the `tail` pointer: if a thread finds that `tail->next != nullptr`, another thread is mid-insert. Rather than spinning, it attempts to advance `tail` forward on the straggler's behalf.

---

### Epoch-Based Memory Reclamation

Every public operation wraps its execution in an `EpochGuard`, which registers the thread as active in the current epoch. Retired nodes are only freed once all threads have advanced past the epoch in which the node was retired. This guarantees that no thread will dereference a freed node, without requiring hazard pointers or reference counting.

---

### Complexity

| Operation           | Time Complexity | Notes                                   |
|---------------------|-----------------|-----------------------------------------|
| `push_front`        | O(1) amortized  | Retries on CAS failure                  |
| `push_back`         | O(1) amortized  | May help advance stale `tail`           |
| `push_at(i)`        | O(i) amortized  | Traversal + CAS retry on conflict       |
| `pop_front`         | O(1) amortized  | Two-phase delete at head                |
| `pop_back`          | O(n)            | Full traversal to second-to-last node   |
| `pop_at(i)`         | O(i) amortized  | Traversal + two-phase delete            |
| `contains` / `find` | O(n)            | Read-only traversal, skips marked nodes |
| `size`              | O(n)            | Full traversal; does not help-advance   |
| `empty`             | O(1)            | Single atomic load on head->next        |

"Amortized" here accounts for CAS retry loops under contention. In the worst case a thread can be repeatedly preempted and retry indefinitely, though in practice contention is bounded by thread count.

---

### Pitfalls and Limitations

**`pop_back` and `size` are O(n).** The same singly-linked list limitation as the fine-grained version applies. Additionally, `size()` intentionally does not call `try_help_advance` to preserve its read-only semantics, so it may traverse logically deleted nodes.

**Pointer marking requires alignment guarantees.** The mark bit trick is only valid because nodes are `alignas(8)`. If this alignment were ever relaxed or the approach ported to a type with smaller alignment, the mark bit would corrupt valid pointer addresses.

**`pop_back` physical deletion is best-effort.** After a successful logical deletion in `pop_back`, the physical CAS to unlink the node from `before_last` is not retried on failure. This is safe because a failure means another thread already helped advance past it, but it means `retire_reference` may be skipped on the primary thread's path — the node will eventually be cleaned up by a helping thread instead.

**`find` returns a raw pointer into the list.** The returned `LinkedListNode<T>*` from `find()` has no lifetime guarantee — the node could be logically deleted and retired by another thread between `find()` returning and the caller dereferencing it. The pointer should be treated as a snapshot hint only, not a stable reference.

**Progress guarantee is obstruction-free, not wait-free.** The retry loops mean a thread under continuous contention is not guaranteed to complete in a bounded number of steps. In practice this is rarely a problem, but it is not a hard wait-free guarantee.

## Fine-Grained Lock Linked List

### Overview

`FGLockLinkedList<T>` is a thread-safe singly-linked list using **fine-grained (per-node) locking**. Rather than protecting the entire list with a single mutex, each node carries its own `std::mutex`, allowing concurrent operations to proceed in parallel on different parts of the list.

The list uses a **sentinel (dummy) head node** that is never removed, which simplifies edge case handling at the front of the list and avoids the need to lock the head pointer itself.

---

### Algorithm — Hand-over-Hand (Chain) Locking

All traversal operations use the **hand-over-hand locking** pattern:

1. Lock the current node.
2. Lock the next node.
3. Unlock the current node.
4. Advance forward.

This ensures that at no point during traversal is a node left unguarded between two concurrent operations, preventing race conditions such as a node being deleted while another thread holds a stale pointer to it.

---

### Node Design

`FGLockNode<T>` is cache-line aligned (`alignas(64)`) to prevent **false sharing** — without this, two threads operating on adjacent nodes could inadvertently invalidate each other's cache lines, causing unnecessary contention and performance degradation.

`operate()` and `dispose()` are thin wrappers around `lock()` and `unlock()` on the node's internal mutex.

---

### Auxiliary Locks

Beyond per-node mutexes, two additional mutexes exist:

- **`tail_mutex`** — guards updates to the `tail` pointer, which is accessed by `push_back`, `push_front` (when inserting into an empty list), `pop_front`, `pop_back`, and `pop_at`.
- **`count_mutex`** — guards the `count` field, updated on every insertion or deletion.

---

### Complexity

| Operation           | Time Complexity | Notes                                     |
|---------------------|-----------------|-------------------------------------------|
| `push_front`        | O(1)            | Locks head node only                      |
| `push_back`         | O(1)            | Locks tail node + `tail_mutex`            |
| `push_at(i)`        | O(i)            | Hand-over-hand traversal to index         |
| `pop_front`         | O(1)            | Locks head + first real node              |
| `pop_back`          | O(n)            | Must traverse to find second-to-last node |
| `pop_at(i)`         | O(i)            | Hand-over-hand traversal to index         |
| `contains` / `find` | O(n)            | Full traversal in the worst case          |
| `size` / `empty`    | O(1)            | Guarded by `count_mutex`                  |

---

### Pitfalls and Limitations

**`pop_back` is O(n).** Because this is a singly-linked list, removing the last element requires a full traversal to find the second-to-last node. This is an inherent limitation — a doubly-linked list would reduce this to O(1) at the cost of more complex locking.

**Lock ordering must be strictly maintained.** All traversal always locks in the forward direction (head → tail). Violating this — e.g. attempting to lock a previous node while holding a later one — would introduce deadlock risk.

**`tail_mutex` and node mutex can be held simultaneously.** In several operations (e.g. `pop_front` when the list has one element), a `std::lock_guard` on `tail_mutex` is acquired while a node mutex is also held. The consistent ordering (node lock acquired before `tail_mutex`) prevents deadlock, but this coupling should be preserved carefully if the code is modified.

**No copy or move semantics.** The list is non-copyable and non-movable by design, as safely transferring ownership of live mutexes and raw node pointers is non-trivial.

**`count` is eventually consistent relative to structural changes.** The count is updated after the structural modification completes, so a `size()` call concurrent with an insertion may transiently return a stale value. This is generally acceptable but worth noting.