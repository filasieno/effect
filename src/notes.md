# IO Ring notes

## `io_uring_params`

*sq_entries*: Submission queue size (number of entries). You specify your desired size, and the kernel may adjust it.

*cq_entries*: Completion queue size. If left as 0, the kernel will set it to double of sq_entries.

*flags*: Configuration flags for io_uring setup. Common flags include:
 - IORING_SETUP_SQPOLL: Enable kernel-side polling
 - IORING_SETUP_IOPOLL: Enable IO polling
 - IORING_SETUP_SQ_AFF: Set CPU affinity for SQ thread

*sq_thread_cpu*: If IORING_SETUP_SQPOLL is set, this specifies which CPU the kernel thread should run on. 

*sq_thread_idle*: The idle timeout for the kernel's submission queue polling thread (in milliseconds).

*features*: Returns supported features from the kernel. Read-only field set by the kernel.

*wq_fd*: File descriptor for sharing kernel worker thread pool between multiple rings.

*sq_off*: Structure containing offsets for various submission queue fields:
- head
- tail
- ring mask
- ring entries
- flags
- dropped
- array

*cq_off*: Structure containing offsets for various completion queue fields:
- head
- tail
- ring mask
- ring entries
- overflow
- cqes
- flags

The offsets represent the memory layout of the *submission queue* (SQ) and *completion queue* (CQ) rings. 

Here's what each field means:    

Submission Queue (SQ):

 - head: Read by kernel, shows where kernel is processing up to
 - tail: Written by user to indicate new submissions
 - ring_mask: Used for fast modulo operations (entries - 1)
 - ring_entries: Total number of entries in the ring
 - flags: Ring state flags
 - dropped: Number of dropped submissions
 - array: Offset to the array of submission queue entries

Completion Queue (CQ):

 - head: Modified by user after processing completions
 - tail: Written by kernel when adding new completions
 - ring_mask: Used for fast modulo operations (entries - 1)
 - ring_entries: Total number of entries in the ring
 - overflow: Tracks number of completion overflows
 - cqes: Offset to the completion queue entries
 - flags: Ring state flags

Regarding your initialization code:

The code looks generally correct and follows the proper initialization sequence for io_uring.

Good practices I see:

Using IORING_FEAT_SINGLE_MMAP check

Proper error handling

Correct calculation of ring buffer sizes

Proper mapping of all necessary pointers

A few suggestions/observations:

 - You're using IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL flags, which means:
 - SQPOLL: Kernel thread will actively poll SQ (higher CPU usage but lower latency)
 - IOPOLL: Polling for completions instead of interrupts (for NVMe drives)

Make sure your system meets the requirements for these flags (especially SQPOLL which needs root privileges)

Consider adding feature checks for these polling modes

The memory mapping sequence is correct:

 - First mapping: SQ and CQ rings
 - Second mapping: SQEs (submission queue entries)

Proper offset calculations for all ring components

The code appears to be a solid implementation of io_uring initialization. Just ensure your use case actually requires the polling modes you've enabled.

```
Mapping 1: Combined SQ/CQ Rings (36928 bytes total)
+--------------------------------------------------------------------------------------+
| Header: Interleaved SQ & CQ Control Fields (0 to ~48 bytes)                          |
|   - SQ head             @ 0  (uint32_t)                                              |
|   - SQ tail             @ 4  (uint32_t)                                              | 
|   - CQ head             @ 8  (uint32_t)                                              | 
|   - CQ tail             @ 12 (uint32_t)                                              |
|   - SQ ring_mask        @ 16 (uint32_t)  [likely 1023 for 1024 entries]              |
|   - CQ ring_mask        @ 20 (uint32_t)  [likely 2047 for 2048 entries]              |
|   - SQ ring_entries     @ 24 (uint32_t)  [1024]                                      |
|   - CQ ring_entries     @ 28 (uint32_t)  [2048]                                      |
|   - SQ dropped          @ 32 (uint32_t)                                              |
|   - SQ flags            @ 36 (uint32_t)  [e.g., bits for IOPOLL/SQPOLL]              |
|   - CQ flags            @ 40 (uint32_t)                                              |
|   - CQ overflow         @ 44 (uint32_t)                                              |
|   (Reserved/padding to 64)                                                           |
+--------------------------------------------------------------------------------------+
| CQEs Array: 2048 × 16 bytes = 32768 bytes (64 to 32832)                              |
|   - CQE[0]  @ 64      [struct io_uring_cqe: user_data (u64), res (s32), flags (u32)] |
|   - CQE[1]  @ 80      [...]                                                          |
|   - ...                                                                              |
|   - CQE[2047] @ 32816 [...]                                                          |
+--------------------------------------------------------------------------------------+
| SQ Index Array: 1024 × 4 bytes = 4096 bytes (32832 to 36928)                         |
|   - Index[0]  @ 32832 (uint32_t)   [points to SQE slot for submission]               |
|   - Index[1]  @ 32836 (uint32_t)   [...]                                             |
|   - ...                                                                              |
|   - Index[1023] @ 36924 (uint32_t) [...]                                             |
+--------------------------------------------------------------------------------------+

Mapping 2: SQEs Array (65536 bytes total)
+-----------------------------------------------------------------------+
| SQEs: 1024 × 64 bytes (0 to 65536)                                    |
|   - SQE[0]  @ 0       [struct io_uring_sqe: opcode (u8), flags (u8),  |
|                        ioprio (u16), fd (s32), off/addr2 (u64),       |
|                        addr/splice_off_in (u64), len (u32),           |
|                        op-specific fields, user_data (u64), etc.]     |
|   - SQE[1]  @ 64      [...]                                           |
|   - ...                                                               |
|   - SQE[1023] @ 65472 [...]                                           |
+-----------------------------------------------------------------------+
```

## Submission state changes

### Initial state

```text
Submission Queue Ring (SQ)
+-----------------+
| head    →  3    |     SQEs Array
| tail    →  3    |    +-------------+
| mask    →  7    |    | 0:  ...     |
| entries → 8     |    | 1:  ...     |
+-----------------+    | 2:  ...     |
                       | 3:  empty   | ← tail points here
Array [0,1,2,3,...]    | 4:  empty   |
                       | 5:  empty   |
                       | 6:  empty   |
                       | 7:  empty   |
                       +-------------+
```                       

### Enqueue request

Steps: 
 1. Read the next SQE index from the SQ ring tail 
 2. get the next SQE index

```c++
int io_submit_file_write(
        IOSystem*   io, 
        int         fd, 
        const void* buff, 
        U64         buff_len, 
        U64         offset, 
        void*       user_data
    ) noexcept {   

    
    U32 sqe_index_unmasked = *io->sring_tail;
    u32 mask = *io->sring_mask;
    U32 sqe_index = sqe_index_unmasked & mask;

    // Add our submission queue entry to the tail of the SQE ring buffer 
    
    
    struct io_uring_sqe *sqe = &io->sqes[sqe_index];
    io->sring_array[sqe_index] = sqe_index;
    sqe_index_unmasked++;

    // Fill in the parameters required for the read or write operation 
    sqe->opcode = IORING_OP_WRITE;
    sqe->fd = fd;
    sqe->addr = (unsigned long) buff;
    sqe->len = buff_len;
    sqe->off = offset;

    // Update the tail 
    io_uring_smp_store_release(io->sring_tail, tail);
    
    // ...

    return ret;
}
```

### After enqueueing request


```text
Submission Queue Ring (SQ)
+-----------------+
| head    →  3    |     SQEs Array
| tail    →  4    |     +------------------+
| mask    →  7    |     | 0:  ...          | 
| entries →  8    |     | 1:  ...          | 
+-----------------+     | 2:  ...          | 
                        | 3:  OPENAT       | ← filled with open request
Array [0,1,2,3,...]     | 4:  empty        | ← new tail points here
                        | 5:  empty        |
                        | 6:  empty        |
                        | 7:  empty        |
                        +------------------+
```

### After `io_ring_enter`


```text
     Kernel Space                User Space
    +------------+              +-----------------+
    |            |              | SQ              |
    | Processing | ←----------- | OPENAT request  |
    | OPENAT     |              |                 |
    |            |              |                 |
    |            |              |                 |
    |  Result    | -----------→ | CQ (completion) |
    |            |              |                 |
    +------------+              +-----------------+ 
```

CQ will eventually contain:

- fd number (positive) on success
- negative errno on failure

## x64 memory model 

### Basic Memory Model Concepts

1. Store Buffer

   - CPUs can buffer writes before committing to main memory
   - Other cores may not immediately see writes
   - Creates potential for memory reordering
2. Memory Ordering x64 supports several ordering types:
   - *Relaxed*: No ordering guarantees
   - *Acquire*: Prevents reads from moving before the operation
   - *Release*: Prevents writes from moving after the operation 
   - *Sequential Consistency*: Full ordering barriers

### Key Memory Operations

```c++
// Atomic store with release semantics
std::atomic<int> x{0};
x.store(1, std::memory_order_release);

// Atomic load with acquire semantics
int val = x.load(std::memory_order_acquire);

// Full memory fence
std::atomic_thread_fence(std::memory_order_seq_cst);
```

### Hardware Guarantees

X64 provides these key guarantees:

1. Aligned atomic operations are always atomic
2. Writes to the same location are observed in program order
3. Memory barriers ensure global visibility

### Common Patterns

#### Producer-Consumer

```c++
std::atomic<int> data{0};
std::atomic<bool> ready{false};

// Producer
data.store(42, std::memory_order_relaxed);
ready.store(true, std::memory_order_release);

// Consumer
if (ready.load(std::memory_order_acquire)) {
    // Safe to read data
    int val = data.load(std::memory_order_relaxed);
}
```

#### Double-Checked Locking

```c++
std::atomic<bool> initialized{false};
std::mutex init_mutex;

void init() {
    if (!initialized.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(init_mutex);
        if (!initialized.load(std::memory_order_relaxed)) {
            // Do initialization
            initialized.store(true, std::memory_order_release);
        }
    }
}
```

### Best Practices

1. Use `std::memory_order_seq_cst` by default unless performance critical 
2. Pair release stores with acquire loads
3. Use relaxed ordering only when you fully understand the consequences
4. Prefer higher-level synchronization (mutex, condition variables) when possible

### Common Pitfalls

1. Missing Memory Orders

```c++
// Wrong - possible reordering
atomic<bool> flag{false};
int data = 0;
data = 42;
flag.store(true); // Need release semantics

// Correct
data = 42;
flag.store(true, std::memory_order_release);
```

2. Incorrect Pairing

```c++
// Wrong - acquire/release mismatch
atomic<int> x{0};
x.store(1, std::memory_order_release);
int val = x.load(std::memory_order_relaxed); // Should be acquire

// Correct
x.store(1, std::memory_order_release);
int val = x.load(std::memory_order_acquire);
```

### Performance Considerations

1. Sequential consistency (memory_order_seq_cst) is most expensive
2. Acquire/Release ordering has moderate cost
3. Relaxed ordering has minimal overhead
4. Memory fences are expensive - use sparingly

### Debug Tips

1. Use thread sanitizer to detect race conditions
2. Enable CPU memory ordering warnings in compiler
3. Test with different memory orders in stress tests
4. Document memory ordering assumptions in code

Remember: When in doubt, use stronger memory ordering guarantees and only optimize when necessary and after thorough testing.



```
The issue is in your suspend effect implementation. When a task suspends, it's transferring control back to the scheduler, but the scheduler code is not properly handling the task resumption flow.

Looking at the code in effects.cc, the SuspendEffect::await_suspend is correctly:

Moving the current task to READY state

Moving the scheduler task to RUNNING state

Returning a handle to the scheduler task

However, in kernel.cc, the scheduler's code is not properly handling the continuation after a task suspends. The scheduler coroutine appears to be restarting from the beginning rather than continuing from where it left off.

The issue is in the scheduler function - after resuming the main task, it continues execution and hits the main loop again without properly handling the suspension point. It should instead pick up from where it left off when a task suspends.

Here's what's happening:

Scheduler starts and spawns main task

Main task runs and hits suspend()

Control transfers back to scheduler via symmetric transfer

Instead of continuing after the main_task.resume() line, the scheduler restarts from the beginning

This causes the started counter to increment again, triggering the assertion

To fix this, you need to restructure the scheduler code to properly handle the continuation after task suspension. The scheduler should:

Have a clear entry point for initial startup

Have a separate path for handling suspended tasks

Properly maintain its state between task switches

The scheduler should look more like this pattern:

kernel.cc 127:130




SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept {    
    // Initial startup - only runs once
    Task main_task = user_main_task();
    main_task.resume();

    // Main scheduling loop - handles suspended tasks
    while (true) {
        if (g_kernel.ready_count > 0) {
            // Handle ready tasks...
        }
        // Handle other states...
    }
    co_return;
}
This way, the scheduler won't restart from the beginning when tasks suspend back to it.
```



```
The Root Cause
In your scheduler() function in kernel.cc, after you resume the main task, you have:
