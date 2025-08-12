#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <cstdint>
#include <cstring>
#include "liburing.h"

namespace ak {
namespace internal {
#ifdef NDEBUG
    constexpr bool IS_DEBUG_MODE    = false;
#else
    constexpr bool IS_DEBUG_MODE    = true;   
#endif
    constexpr bool TRACE_DEBUG_CODE = false;
}

// -----------------------------------------------------------------------------

/// \defgroup Task Task API
/// \brief Task API defines the API for creating and managing tasks.

/// \defgroup Kernel Kernel API
/// \brief Kernel API defines system level APIs.

// -----------------------------------------------------------------------------

using U64  = __u64;  
using U32  = __u32; 
using Size = __SIZE_TYPE__; 

/// \brief Idenfies the state of a task
/// \ingroup Task
enum class TaskState
{
    INVALID = 0, ///< Invalid OR uninitialized state
    CREATED,     ///< Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
    READY,       ///< Ready for execution
    RUNNING,     ///< Currently running
    IO_WAITING,  ///< Waiting for IO
    WAITING,     ///< Waiting for Critical Section
    ZOMBIE,      ///< Already dead
    DELETING     ///< Currently being deleted
};

inline const char* ToString(TaskState state) noexcept 
{
    switch (state) {
        case TaskState::INVALID:    return "INVALID";
        case TaskState::CREATED:    return "CREATED";
        case TaskState::READY:      return "READY";
        case TaskState::RUNNING:    return "RUNNING";
        case TaskState::IO_WAITING: return "IO_WAITING";
        case TaskState::WAITING:    return "WAITING";
        case TaskState::ZOMBIE:     return "ZOMBIE";
        case TaskState::DELETING:   return "DELETING";
        default: return nullptr;
    }
}

struct DefineTask;

struct TaskContext;

/// \brief Coroutine handle for a Task
/// \ingroup Task
using TaskHdl = std::coroutine_handle<TaskContext>;

/// \brief Defines a Task function type-erased pointer (no std::function)
/// \ingroup Task
template <typename... Args>
using TaskFn = DefineTask(*)(Args...);

namespace internal {

    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

    struct DLink {
        DLink* next;
        DLink* prev;
    };

    inline void InitLink(DLink* link) {
        link->next = link;
        link->prev = link;
    }

    inline bool IsLinkDetached(const DLink* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        return link->next == link && link->prev == link;
    }

    inline void DetachLink(DLink* link) {
        assert(link != nullptr);
        assert(link->next != nullptr);
        assert(link->prev != nullptr);
        if (IsLinkDetached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline void EnqueueLink(DLink* queue, DLink* link) {
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        assert(IsLinkDetached(link));

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* DequeueLink(DLink* queue) {
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        if (IsLinkDetached(queue)) return nullptr;
        DLink* target = queue->prev;
        DetachLink(target);
        return target;
    }

    inline void InsertPrevLink(DLink* queue, DLink* link) {
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        assert(IsLinkDetached(link));

        link->next = queue;
        link->prev = queue->prev;

        link->next->prev = link;
        link->prev->next = link;
    }

    inline void InsertNextLink(DLink* queue, DLink* link) {
        assert(link != nullptr);
        assert(queue->next != nullptr);
        assert(queue->prev != nullptr);
        assert(IsLinkDetached(link));

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    /// \brief AllocState is the state of the allocation.
    /// \ingroup Allocator
    /// 0 -> invalid
    /// first bit  -> is free
    /// second bit -> free
    /// third bit  -> sentinel
    enum class AllocState {
        INVALID              = 0b0000,
        USED                 = 0b0010,
        FREE                 = 0b0001,
        WILD_BLOCK           = 0b0011,
        BEGIN_SENTINEL       = 0b0100,
        LARGE_BLOCK_SENTINEL = 0b0110,
        END_SENTINEL         = 0b1100,
    };
    
    struct AllocSizeRecord {
        U64 size      : 48;
        U64 state     : 4;
        U64 _reserved : 12;
    };

    constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
    constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
    constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

    struct AllocHeader {
        AllocSizeRecord thisSize;
        AllocSizeRecord prevSize;
    };

    struct FreeAllocHeader : public AllocHeader {
        DLink freeListLink;
    };
    static_assert(sizeof(FreeAllocHeader) == 32);

    static constexpr int ALLOCATOR_BIN_COUNT = 256;
    
    struct AllocStats {
        Size binAllocCount[ALLOCATOR_BIN_COUNT];
        Size binFreeCount[ALLOCATOR_BIN_COUNT];
        Size binSplitCount[ALLOCATOR_BIN_COUNT];
        Size binMergeCount[ALLOCATOR_BIN_COUNT];
        Size binReuseCount[ALLOCATOR_BIN_COUNT];
    };
    
    /// \brief AllocTable is the main data structure for our general purpose allocator.
    /// 
    /// The allocator is designed for single-threaded operation and implements a segregated 
    /// free list strategy with 256 bins for efficient memory allocation and deallocation.
    /// It tracks heap boundaries, free list state, and comprehensive statistics.
    ///  
    /// ## Architecture Overview
    /// 
    /// The AllocTable serves as the central control structure for the allocator, managing:
    /// - 256 segregated free list bins for different allocation sizes
    /// - SIMD-optimized bin mask for fast free bin identification  
    /// - Heap boundary tracking and memory usage statistics
    /// - Sentinel blocks for safe memory traversal
    /// - Performance counters for allocation profiling
    /// 
    /// ## Bin Organization (256 bins total)
    /// 
    /// All allocations are aligned to 32-byte boundaries for optimal cache performance.
    /// The 256 bins are organized as follows:
    /// 
    /// - **Small bins (0-253)**: Fixed-size bins for common allocations
    ///   - Bin 0: 32 bytes
    ///   - Bin 1: 64 bytes  
    ///   - Bin 2: 96 bytes
    ///   - Bin 3: 128 bytes
    ///   - ...
    ///   - Bin 253: 8,096 bytes (32 * 253)
    /// 
    /// - **Medium bin (254)**: Variable-size bin for medium blocks (8,128+ bytes)
    ///   Used for blocks too large for small bins but not requiring wild block allocation
    /// 
    /// - **Wild block bin (255)**: Special bin containing the wild block
    ///   A large contiguous region used for very large allocations when no suitable
    ///   free blocks exist in other bins
    /// 
    /// ## SIMD-Accelerated Bin Selection
    /// 
    /// The allocator leverages AVX2 SIMD instructions for extremely fast bin selection:
    /// 
    /// 1. **Bit Mask Representation**: The `freeListbinMask` (__m256i) contains 256 bits,
    ///    one for each bin. A set bit indicates the bin contains free blocks.
    /// 
    /// 2. **Fast Bin Finding**: To find the first suitable bin for size S (branchless):
    ///    ```cpp
    ///    unsigned targetBin = (S + 31) >> 5;  // Round up to bin index (divide by 32)
    ///    
    ///    // Branchless mask creation: set all bits from targetBin to 255
    ///    // Each 64-bit segment handles bins [0-63], [64-127], [128-191], [192-255]
    ///    unsigned shift0 = targetBin & 63;           // targetBin % 64
    ///    unsigned shift1 = (targetBin - 64) & 63;    // (targetBin - 64) % 64  
    ///    unsigned shift2 = (targetBin - 128) & 63;   // (targetBin - 128) % 64
    ///    unsigned shift3 = (targetBin - 192) & 63;   // (targetBin - 192) % 64
    ///    
    ///    // Create masks using arithmetic right shift to avoid branches
    ///    uint64_t maskLow = (~0ULL << shift0) & (((int64_t)(63 - targetBin)) >> 63);
    ///    uint64_t maskHigh = (~0ULL << shift1) & (((int64_t)(127 - targetBin)) >> 63);
    ///    uint64_t maskHigh2 = (~0ULL << shift2) & (((int64_t)(191 - targetBin)) >> 63);
    ///    uint64_t maskHigh3 = (~0ULL << shift3) | (1ULL << 63); // Wild block always set
    ///    
    ///    __m256i clearMask = _mm256_setr_epi64x(maskLow, maskHigh, maskHigh2, maskHigh3);
    ///    
    ///    // Apply mask and find first set bit (first available bin >= targetBin)
    ///    __m256i availableBins = _mm256_and_si256(freeListbinMask, clearMask);
    ///    unsigned availableBin = _tzcnt_u32(_mm256_movemask_epi8(availableBins));
    ///    ```
    /// 
    /// 3. **Wild Block Guarantee**: Bit 255 (wild block) is always set to 1, ensuring
    ///    that there's always at least one available bin for any allocation size.
    /// 
    /// 4. **Branchless Performance**: This completely branchless implementation provides:
    ///    - O(1) bin selection regardless of allocation size or number of bins
    ///    - No conditional jumps or pipeline stalls
    ///    - Predictable execution time for all allocation sizes
    ///    - Optimal CPU pipeline utilization compared to O(n) linear search
    /// 
    /// ### Lane-wise Find-First Strategy (x64 AVX2/BMI1)
    /// 
    /// The 256-bit availability mask is logically four 64-bit lanes. The find-first
    /// non-empty bin is derived lane-wise without branches:
    /// 
    /// 1. Build a lane-masked view by AND-ing `freeListbinMask` with the branchless
    ///    cut mask created from `targetBin` (see above).
    /// 2. Extract per-lane 64-bit values (conceptually) and compute per-lane
    ///    "is-zero" flags using vector test operations (e.g., `_mm256_testz_si256`) or
    ///    comparisons combined via bitwise ops.
    /// 3. For the first non-zero lane, compute `tzcnt64(lane)` and add its lane
    ///    offset in {0, 64, 128, 192} to obtain the global bin index.
    /// 4. Use boolean masks to select the winning lane/index without control-flow
    ///    branches; on x64, BMI1 `tzcnt` provides constant-time trailing-zero count.
    /// 
    /// Example (illustrative, simplified):
    /// ```cpp
    /// // masked is the 256-bit availability after clearing bins < targetBin
    /// __m256i masked = _mm256_and_si256(freeListbinMask, clearMask);
    /// 
    /// // Derive 64-bit lane values (conceptually; compilers lower this efficiently)
    /// uint64_t l0 = (uint64_t)_mm256_extract_epi64(masked, 0);
    /// uint64_t l1 = (uint64_t)_mm256_extract_epi64(masked, 1);
    /// uint64_t l2 = (uint64_t)_mm256_extract_epi64(masked, 2);
    /// uint64_t l3 = (uint64_t)_mm256_extract_epi64(masked, 3);
    /// 
    /// // Compute candidate indices per lane (tzcnt undefined on 0 -> guard via masks)
    /// unsigned i0 = _tzcnt_u64(l0) + 0u;
    /// unsigned i1 = _tzcnt_u64(l1) + 64u;
    /// unsigned i2 = _tzcnt_u64(l2) + 128u;
    /// unsigned i3 = _tzcnt_u64(l3) + 192u;
    /// 
    /// // Lane non-emptiness masks (0xFFFFFFFF if lane non-zero, else 0)
    /// uint32_t m0 = (uint32_t)-(l0 != 0);
    /// uint32_t m1 = (uint32_t)-(l1 != 0);
    /// uint32_t m2 = (uint32_t)-(l2 != 0);
    /// uint32_t m3 = (uint32_t)-(l3 != 0);
    /// 
    /// // Select the first non-empty lane without branches
    /// unsigned idx = (m0 & i0) | (~m0 & (m1 & i1 | (~m1 & (m2 & i2 | (~m2 & i3)))));
    /// ```
    /// 
    /// Portability: This implementation targets x86-64 with AVX2 and BMI1 (tzcnt).
    /// Portability/fallback paths are explicitly out of scope for this design.
    /// 
    /// ## Allocation Strategy
    /// 
    /// This allocator targets O(1) bin selection with a large number of bins to
    /// reduce fragmentation, and it is designed to run safely under near
    /// memory-exhaustion conditions:
    /// 
    /// - **O(1) bin selection with SIMD**: The target bin is determined in constant
    ///   time using a single SIMD AND plus trailing-zero-count. The large number of
    ///   bins (256) provides fine-grained size classes, minimizing internal
    ///   fragmentation while keeping selection cost constant.
    /// 
    /// - **Coroutine-based allocations (co_await)**: Allocations are requested from
    ///   within coroutines. If sufficient memory is available, allocation completes
    ///   immediately. If not, the requesting coroutine suspends (via `co_await`) and
    ///   is resumed automatically when memory becomes available. This avoids
    ///   allocation failures while maintaining forward progress.
    /// 
    /// - **No external coordination structures**: The system avoids auxiliary queues
    ///   or buffers that would require extra allocations. This eliminates "allocate to
    ///   allocate" scenarios and reduces memory pressure precisely when memory is
    ///   scarce.
    /// 
    /// - **Fixed-heap operation (initial design)**: The initial implementation assumes
    ///   a fixed-size heap. Under pressure, operations do not fail; instead,
    ///   coroutines naturally throttle by suspending until free memory is returned to
    ///   the allocator.
    /// 
    /// Practical flow:
    ///
    /// 1. Compute target bin in O(1) with SIMD  
    /// 2. If a suitable free block exists, allocate and return  
    /// 3. Otherwise, suspend the coroutine; resume it when memory is freed  
    /// 4. Coalescing on free improves availability and reduces fragmentation
    /// 
    /// ## Wild Block Strategy
    /// 
    /// The wild block serves as the allocator's "reservoir" for large allocations:
    /// 
    /// - **Purpose**: Handles allocations larger than 8,096 bytes or when no suitable
    ///   free blocks exist in regular bins
    /// - **Management**: Maintained as a single large contiguous region in bin 255
    /// - **Splitting**: When used, the wild block is split and remainder stays as wild block
    /// - **Coalescing**: Adjacent free blocks are merged back into the wild block when possible
    /// - **Fragmentation Control**: Minimizes external fragmentation by providing a
    ///   fallback for unusual allocation sizes
    /// 
    /// ## Free List Implementation
    /// 
    /// Each bin uses an intrusive doubly-linked list for optimal performance:
    /// 
    /// - **Storage**: List pointers stored in the free block's payload area (no overhead)
    /// - **Structure**: Circular lists with sentinel heads for consistent operations
    /// - **Performance**: O(1) insertion, removal, and empty checking
    /// - **Cache Efficiency**: 64-byte alignment prevents false sharing between bins
    ///
    /// # Large blocks
    ///
    /// Large blocks are allocated are allocated past the  **large block sentinel** but before the **end block sentinel**
    /// Large blocks do not have a header, but are managed using an external table.
    /// Usually large blocks are allocated at the beginning of the software operation and stay allocated until the end.
    ///
    /// ## Wild block
    ///
    /// The wild block is a large contiguous region of memory that is used to allocate
    /// large blocks.
    ///
    /// 
    /// ## Thread Safety
    /// 
    /// **IMPORTANT**: This allocator is designed for single-threaded use only.
    /// No synchronization primitives are used. For multi-threaded applications,
    /// either use external locking or consider per-thread allocator instances.
    /// 
    /// ## Usage Example
    /// 
    /// Synchronous allocation with possible failure:
    ///
    /// ```cpp
    /// // Initialize allocator with 1MB heap
    /// AllocTable allocTable;
    /// void* heap = mmap(nullptr, 1024*1024, PROT_READ|PROT_WRITE, 
    ///                   MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    /// InitAllocTable(&allocTable, heap, 1024*1024);
    /// 
    /// // Allocate 128 bytes (goes to bin 3)
    /// void* ptr = TryAllocMem(&allocTable, 128);
    /// assert(ptr != nullptr);
    /// // Free the allocation (triggers coalescing if possible)
    /// FreeMem(&allocTable, ptr);
    /// 
    /// // Query statistics
    /// printf("Allocations: %zu, Fragmentation: %.2f%%\n", 
    ///        allocTable.totalAllocCount,
    ///        (1.0 - (double)allocTable.freeMemSize / allocTable.memSize) * 100);
    /// ```
    ///
    /// Coroutine-based allocation for systems that run close to memory exhaustion:
    ///
    /// ```cpp
    /// DefineTask ProcessData() {
    ///     void* buffer = co_await AllocMem(4096);  // Suspend if no memory
    ///     // ... process data ...
    ///     FreeAsync(buffer);  // May resume waiting coroutines
    /// }
    /// ```
    /// 
    /// ## Performance Characteristics
    /// 
    /// - **Allocation**: O(1) average case, O(log n) worst case for large allocations
    /// - **Deallocation**: O(1) with immediate coalescing
    /// - **Memory Overhead**: ~6.25% (16-byte headers on 32-byte aligned blocks)
    /// - **Fragmentation**: Minimized through segregated bins and coalescing
    /// - **Cache Performance**: Optimized with 64-byte alignments and SIMD operations
    /// 
    /// ## Statistics and Monitoring
    /// 
    /// The AllocTable provides comprehensive statistics for performance analysis:
    /// 
    /// ### Global Counters
    /// - `totalAllocCount`: Track allocation frequency and patterns
    /// - `totalFreeCount`: Monitor deallocation behavior and potential leaks
    /// - `totalReallocCount`: Identify memory resize patterns
    /// - `totalSplitCount`: Measure fragmentation from block splitting
    /// - `totalMergeCount`: Track defragmentation effectiveness
    /// - `totalReuseCount`: Monitor free list efficiency
    /// 
    /// ### Memory Usage Tracking
    /// - `usedMemSize` / `freeMemSize`: Real-time memory utilization
    /// - `memSize` vs `requestedMemSize`: Heap growth analysis
    /// - `maxMemSize`: Memory limit enforcement
    /// 
    /// ### Per-Bin Statistics (via AllocStats)
    /// - `binAllocCount[i]`: Allocation frequency per bin size
    /// - `binFreeCount[i]`: Deallocation patterns per bin
    /// - `binSplitCount[i]`: Fragmentation analysis per size class
    /// - `binMergeCount[i]`: Coalescing effectiveness per bin
    /// - `binReuseCount[i]`: Free list hit rate per bin
    /// 
    /// ### Derived Metrics
    /// ```cpp
    /// // Memory utilization percentage
    /// double utilization = (double)usedMemSize / memSize * 100;
    /// 
    /// // Average allocation size
    /// double avgAllocSize = (double)usedMemSize / totalAllocCount;
    /// 
    /// // Fragmentation estimate (external)
    /// double fragmentation = 1.0 - (largestFreeBlock / freeMemSize);
    /// 
    /// // Allocation/Free balance (should approach 1.0)
    /// double balance = (double)totalFreeCount / totalAllocCount;
    /// ```
    /// 
    /// ## Comparison with Other Single-Threaded Allocators
    /// 
    /// This allocator is optimized for constant-time bin selection, minimal
    /// fragmentation, and robust behavior near memory exhaustion via coroutine
    /// suspension (no external coordination structures).
    /// 
    /// - TLSF (Two-Level Segregated Fit):
    ///   - Strengths: Deterministic O(1) alloc/free, good fragmentation profile.
    ///   - Compared to this design: Similar asymptotics, but SIMD-based 256-bin
    ///     selection here achieves lower instruction count on hot paths and finer
    ///     size classes. Near OOM, TLSF must overprovision or fail; here, coroutines
    ///     suspend without extra structures, avoiding "allocate to allocate".
    /// 
    /// - dlmalloc:
    ///   - Strengths: Battle-tested, general-purpose.
    ///   - Compared to this design: Typically higher bin lookup cost and more
    ///     fragmentation. No built-in coroutine suspension model; requires
    ///     overprovisioning or failure when memory is tight.
    /// 
    /// - jemalloc/tcmalloc (single-threaded use of a single arena):
    ///   - Strengths: Excellent throughput and fragmentation in general-purpose use.
    ///   - Compared to this design: Larger machinery, often tuned for multi-threaded
    ///     workloads. This allocator’s fixed-heap, SIMD O(1) selection and
    ///     coroutine suspension provide more predictable latency and graceful
    ///     behavior near OOM without extra coordination buffers.
    /// 
    /// - rpmalloc/mimalloc (single-threaded mode):
    ///   - Strengths: Very fast small/medium-size alloc/free paths.
    ///   - Compared to this design: Small-object fast paths typically rely on
    ///     caches or slabs that require headroom. Under near-exhaustion, they can
    ///     require overprovisioning to remain stable. Here, intentional omission of
    ///     a growable small-object pool avoids extra memory pressure; the system
    ///     instead throttles via coroutine suspension.
    /// 
    /// Key advantages of this near-OOM design:
    /// - **No overprovisioning required**: The system remains operational by
    ///   suspending requesters; memory pressure doesn’t cascade into failures.
    /// - **No external coordination allocations**: Wait semantics are intrinsic to
    ///   coroutines, avoiding additional buffers/queues.
    /// - **O(1) bin selection at scale**: 256 bins plus branchless SIMD yields
    ///   constant-time selection with fine granularity to reduce fragmentation.
    /// - **Fixed-heap predictability**: No OS calls in the hot path; latency is
    ///   stable even under pressure.
    /// 
    /// ## Allocator Rating Card (x86-64, single-threaded)
    /// 
    /// - Throughput (steady-state, medium sizes): 9/10 — O(1) SIMD bin select + O(1) free/coalesce
    /// - Tail Latency Predictability: 9/10 — Branchless hot path; no syscalls in steady state
    /// - Near-OOM Robustness: 10/10 — Coroutine suspension; no overprovisioning required
    /// - Internal Fragmentation: 8/10 — 256 bins, 32-byte granularity balance
    /// - External Fragmentation: 8/10 — Immediate coalescing + wild block reservoir
    /// - Memory Overhead: 8/10 — Compact headers; 32B alignment trade-off
    /// - Simplicity/Maintainability: 8/10 — Clear structure; SIMD path requires care
    /// - Determinism (single-threaded): 9/10 — Fixed-heap, constant-time selection
    /// - Portability: N/A by design — x86-64 AVX2/BMI1 focus
    /// 
    /// ## Possible Improvements
    /// 
    /// - SIMD lane selection micro-optimizations:
    ///   - Use lane-zero tests with `_mm256_testz_si256` and BMI1 `tzcnt` per 64-bit lane,
    ///     then combine via bitwise masks to remain branchless.
    ///   - Optionally precompute 256 cut-masks (2 KB table) to avoid per-alloc shifts
    ///     when targetBin frequency is high and predictable.
    /// 
    /// - TargetBin clamping and invariants:
    ///   - Ensure targetBin is clamped to [0, 255] and bit 255 (wild block) stays set.
    ///   - Add lightweight asserts to validate mask invariants in debug builds.
    /// 
    /// - Wait-list policies under pressure:
    ///   - Introduce size-bucketed wait lists and/or aging to avoid starvation.
    ///   - Batch wakeups on large frees/merges to reduce scheduler thrash.
    /// 
    /// - Large-block policy refinements (future OS-backed mode):
    ///   - Page-align large blocks and track lifetime classes to reduce external fragmentation.
    ///   - Consider `madvise`-style hints (when OS backing exists) for long-lived large blocks.
    /// 
    /// - Optional fixed small-object fast path (non-growable):
    ///   - A compile-time-sized, non-growable slab carved at init can provide a fast path
    ///     for 32–256 B objects without violating near-OOM goals; when exhausted, allocations
    ///     naturally fall back to the general path and suspension semantics.
    /// 
    /// - Instrumentation and observability:
    ///   - Add tracepoints for suspend/resume, split/merge, and wild-block usage.
    ///   - Maintain short-term histograms for allocation sizes to guide tuning.
    ///
    /// ## FF Allocator vs Prior Art
    ///
    /// - Known ideas
    ///   - O(1) binning (TLSF), boundary tags, wild/reservoir blocks: known.
    ///   - Bitmaps for size classes: known; some allocators use bit twiddling, but SIMD-accelerated first-fit with 256 bins + branchless masks is not widely documented.
    ///   - Sleepable allocations: common in kernels (e.g., Linux GFP_KERNEL can sleep; slab allocators wake sleepers), rare in general-purpose user-space allocators.
    /// - Differentiators
    ///   - Coroutine-native allocation that suspends under pressure (no external queues); “no allocate-to-allocate.”
    ///   - Fixed-heap operation tuned for near-OOM regimes with built-in backpressure.
    ///   - Branchless, lane-wise SIMD (AVX2/BMI1) bin selection at 256 bins to reduce fragmentation while keeping O(1).
    /// “Has anyone done this?”
    /// - Closest parallels
    ///   - Kernel allocators that may block on memory, then wake sleepers on free.
    ///   - Research/industrial allocators with bitmap-based bin finding (some use popcnt/ctz), but few (if any) publicly emphasize SIMD+branchless selection at this granularity.
    ///   - Task systems with memory-aware throttling/backpressure, but not at the allocator call boundary as an awaitable primitive.
    /// - Likely novelty
    ///   - The coroutine-suspending user-space allocator as a first-class API plus a branchless SIMD O(1) selection path appears novel enough for a workshop/experience paper.
    /// - Contributions
    ///   - O(1) branchless SIMD bin selection with 256 bins and a lane-wise first-set protocol.
    ///   - Coroutine-native alloc API that suspends and resumes without external coordination or extra allocations.
    ///   - Robust near‑OOM behavior: no overprovisioning; automatic backpressure; predictable tail latency.
    /// - Evaluation (x86-64, single-threaded)
    ///   - Throughput/latency vs TLSF, dlmalloc, mimalloc/rpmalloc (single arena), jemalloc/tcmalloc (single arena).
    ///   - Fragmentation (internal/external), split/merge counts, largest-free vs total-free under adversarial and realistic traces.
    ///   - Near‑OOM benchmarks: survival curves, request wait times, fairness, bursty free/wakeup behavior, tail latencies.
    ///   - Ablations: with/without SIMD, with/without suspension, varying bin counts, wildcard reservoir on/off.
    /// - Proofs/assurances
    ///   - Correctness invariants (headers, coalescing, sentinels).
    ///   - SIMD lane correctness and tzcnt fallbacks not required (x86-64 only, by design).
    ///
    struct AllocTable {
        
        // ==================== FREE LIST MANAGEMENT ====================
        
        /// \brief SIMD bit mask for fast bin availability lookup
        /// 
        /// 256-bit AVX2 register containing availability bits for all 256 bins.
        /// Each bit indicates whether the corresponding bin contains free blocks.
        /// Aligned to 64-byte boundary for optimal SIMD performance.
        /// Used with SIMD instructions to find the first available bin in O(1) time.
        alignas(64) U32 freeListbinMask[8];                         
        
        /// \brief Array of doubly-linked list heads for each bin
        /// 
        /// Each DLink serves as the head/sentinel for a bin's free block list.
        /// Free blocks in each bin are organized as circular doubly-linked lists
        /// for O(1) insertion and removal. Aligned to 64-byte boundary to prevent
        /// false sharing and optimize cache performance when accessing multiple bins.
        alignas(64) DLink freeListBins[ALLOCATOR_BIN_COUNT];
        
        /// \brief Size tracking for each bin's free blocks
        /// 
        /// Contains the total size (in bytes) of all free blocks in each bin.
        /// Used for statistics, fragmentation analysis, and allocation strategy
        /// optimization. Aligned to 64-byte boundary for cache efficiency.
        alignas(64) U32 freeListBinsCount[ALLOCATOR_BIN_COUNT];

        // ==================== HEAP BOUNDARY MANAGEMENT ====================
        
        /// \brief Start of the raw heap memory region
        /// 
        /// Points to the beginning of the heap as returned by the system allocator
        /// (e.g., mmap, sbrk). May not be aligned to allocator requirements.
        /// This is the address that must be passed to the system deallocator.
        alignas(8) char* heapBegin;
        
        /// \brief End of the heap memory region  
        /// 
        /// Points to the first byte beyond the heap. The valid heap range is
        /// [heapBegin, heapEnd). Used for bounds checking during allocation
        /// and to determine when heap expansion is needed.
        alignas(8) const char* heapEnd;
        
        /// \brief Start of aligned memory region for allocations
        /// 
        /// Points to heapBegin aligned up to 32-byte boundary. All allocator
        /// blocks are carved from the region [memBegin, memEnd). This ensures
        /// all allocated blocks maintain proper 32-byte alignment.
        alignas(8) char* memBegin;
        
        /// \brief End of aligned memory region for allocations
        /// 
        /// Points to memBegin aligned up to 32-byte boundary. All allocator
        /// blocks are carved from the region [memBegin, memEnd). This ensures
        /// all allocated blocks maintain proper 32-byte alignment.
        alignas(8) char* memEnd;
        
        // ==================== MEMORY ACCOUNTING ====================
        
        /// \brief Current total heap size
        /// 
        Size memSize;
        
        /// \brief Currently allocated memory
        /// 
        /// Total bytes currently allocated to users (not including headers).
        /// Updated on each allocation and deallocation for real-time tracking.
        Size usedMemSize;
        
        /// \brief Currently free memory
        /// 
        /// Total bytes available for allocation across all bins and wild block.
        /// Should satisfy: usedMemSize + freeMemSize ≈ memSize (accounting for headers).
        Size freeMemSize;

        /// \brief Maximum free block size
        /// 
        /// The largest free block size in the heap.
        Size maxFreeBlockSize;
        
        // ==================== ALLOCATION STATISTICS ====================
        
        /// \brief Total number of successful allocations
        /// 
        /// Incremented on each successful malloc/alloc call. Used for performance
        /// analysis and allocation pattern profiling.
        Size totalAllocCount;
        
        /// \brief Total number of deallocations
        /// 
        /// Incremented on each free call. Should eventually equal totalAllocCount
        /// in programs that properly free all allocations.
        Size totalFreeCount;
        
        /// \brief Total number of reallocation operations
        /// 
        /// Incremented on each realloc call. Tracks memory resize operations
        /// which may involve copying data to larger blocks.
        Size totalReallocCount;
        
        /// \brief Total number of block splits performed
        /// 
        /// Incremented when a free block is split to satisfy a smaller allocation.
        /// High split counts may indicate fragmentation or suboptimal bin sizing.
        Size totalSplitCount;
        
        /// \brief Total number of block merges performed
        /// 
        /// Incremented when adjacent free blocks are coalesced during deallocation.
        /// Indicates the allocator's effectiveness at reducing fragmentation.
        Size totalMergeCount;
        
        /// \brief Total number of block reuses from free lists
        /// 
        /// Incremented when an allocation is satisfied by reusing a free block
        /// from a bin rather than splitting or expanding the heap.
        Size totalReuseCount;
        
        // ==================== SENTINEL BLOCKS ====================
        
        /// \brief Sentinel block at the beginning of heap
        /// 
        /// Special marker block placed at memBegin that serves multiple purposes:
        /// - **Boundary Protection**: Prevents backward traversal beyond heap start
        /// - **Coalescing Simplification**: Eliminates special cases in merge logic
        /// - **Traversal Safety**: Provides a consistent starting point for heap walks
        /// - **Never Allocated**: Marked as permanently allocated to prevent use
        /// 
        /// The beginSentinel has zero payload size and is never included in free lists.
        alignas(8) FreeAllocHeader* beginSentinel;
        
        /// \brief Pointer to the wild block
        /// 
        /// Points to the large contiguous free region used for allocations
        /// that cannot be satisfied by regular bins. The wild block characteristics:
        /// - **Dynamic Size**: Shrinks as allocations are carved from it
        /// - **Fallback Allocation**: Used when no suitable bins are available
        /// - **Coalescing Target**: Free blocks adjacent to wild block merge into it
        /// - **Null When Fragmented**: May be null if heap is highly fragmented
        /// - **Bin 255 Resident**: Always stored in the last bin when available
        alignas(8) FreeAllocHeader* wildBlock;
        
        /// \brief Sentinel for large block tracking
        /// 
        /// Special marker used to manage very large allocations (typically >64KB)
        /// that bypass the normal binning system. Functions include:
        /// - **Large Block Chain**: Links together oversized allocations
        /// - **Direct Allocation Tracking**: Monitors blocks allocated directly from system
        /// - **Heap Traversal Aid**: Helps navigate around large blocks during walks
        /// - **Statistics Collection**: Enables separate accounting for large allocations
        alignas(8) FreeAllocHeader* largeBlockSentinel;
        
        /// \brief Sentinel block at the end of heap
        /// 
        /// Special marker block placed at heapEnd that provides heap boundary control:
        /// - **Forward Traversal Limit**: Prevents walking beyond heap end
        /// - **Coalescing Boundary**: Stops merge operations at heap edge
        /// - **Growth Point**: Marks where heap expansion occurs
        /// - **Consistency Check**: Validates heap structure during debugging
        /// 
        /// Like beginSentinel, this block is never allocated and has zero payload.
        alignas(8) FreeAllocHeader* endSentinel;
        
        // TODO: 

    };

    struct Kernel {
        alignas(64) AllocTable allocTable;
        
        // Hot scheduling data (first cache line)
        TaskHdl currentTaskHdl;
        TaskHdl schedulerTaskHdl;
        DLink   readyList;
        DLink   taskList;
        void*   mem;
        Size    memSize;
        
        alignas(64) 
        io_uring ioRing;
        unsigned ioEntryCount;
        
        alignas(64) 
        KernelTaskHdl kernelTask;
        DLink         zombieList;

        alignas(64) 
        int taskCount;
        int readyCount;
        int waitingCount;
        int ioWaitingCount;
        int zombieCount;
        int interrupted;
    };
    
    extern struct Kernel gKernel;

    struct InitialSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        void           await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {}
    };

    struct FinalSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept { assert(false); }
    };

    struct ResumeTaskOp {
        explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};

        constexpr bool await_ready() const noexcept { return hdl.done();}
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl hdl;
    };

    struct JoinTaskOp {
        explicit JoinTaskOp(TaskHdl hdl) : joinedTaskHdl(hdl) {};

        constexpr bool await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl joinedTaskHdl;
    };

    struct SuspendOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {};
    };

    struct GetCurrentTaskOp {
        constexpr bool    await_ready() const noexcept        { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept { this->hdl = hdl; return hdl; }
        constexpr TaskHdl await_resume() const noexcept       { return hdl; }

        TaskHdl hdl;
    };

    void CheckInvariants() noexcept;
    
    void DebugTaskCount() noexcept;
    
}

/// \brief Define a context.
/// \ingroup Task
struct TaskContext {
    using Link = internal::DLink;

    void* operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    void  operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    template <typename... Args>
    TaskContext(Args&&... ) {
        using namespace internal;

        InitLink(&taskListLink);
        InitLink(&waitLink);
        InitLink(&awaitingTerminationList);
        state = TaskState::CREATED;
        enqueuedIO = 0;
        ioResult = -1;

        // Check post-conditions
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    ~TaskContext() {
        using namespace internal;
        assert(state == TaskState::DELETING);
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        DebugTaskCount();
        CheckInvariants();
    }

    TaskHdl        get_return_object() noexcept    { return TaskHdl::from_promise(*this);}
    constexpr auto initial_suspend() noexcept      { return internal::InitialSuspendTaskOp{}; }
    constexpr auto final_suspend() noexcept        { return internal::FinalSuspendTaskOp{}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept  { assert(false); }

    TaskState state;
    int       ioResult;
    unsigned  enqueuedIO;
    Link      waitLink;                // Used to enqueue tasks waiting for Critical Section
    Link      taskListLink;            // Global Task list
    Link      awaitingTerminationList; // The list of all tasks waiting for this task
};

/// \brief Marks a Task coroutine function
struct DefineTask {
    using promise_type = TaskContext;

    DefineTask(const TaskHdl& hdl) : hdl(hdl) {}
    operator TaskHdl() const noexcept { return hdl; }

    TaskHdl hdl;
};


/// \brief Clears the target TaskHdl
/// \param hdl the handle to be cleared
/// \ingroup Task
inline void ClearTaskHdl(TaskHdl* hdl) noexcept {
    *hdl = TaskHdl{};
}

/// \brief Checks is the a TaskHdl is valid
/// \param hdl the handle to be cleared
/// \ingroup Task
inline bool IsTaskHdlValid(TaskHdl hdl) {
    return hdl.address() != nullptr;
}

/// \brief Returns the TaskPromise associated with the target TaskHdl
/// @param hdl 
/// @return the TaskPromise associated with the target TaskHdl
/// \ingroup Task
inline TaskContext* GetTaskContext(TaskHdl hdl) {
    return &hdl.promise();
}

/// \brief Returns the TaskPromise associated with the target TaskHdl
/// @param hdl 
/// @return the TaskPromise associated with the target TaskHdl
/// \ingroup Task
inline TaskContext* GetTaskContext() {
    return &internal::gKernel.currentTaskHdl.promise();
}

/// \brief Get the current Task
/// \return [Async] TaskHdl
/// \ingroup Task
inline constexpr auto GetCurrentTask() noexcept {
    return internal::GetCurrentTaskOp{};
}

/// \brief Suspends the current Task and resumes the Scheduler.
/// \return [Async] void
/// \ingroup Task
inline constexpr auto SuspendTask() noexcept { return internal::SuspendOp{}; }

/// \brief Suspends the current Task until the target Task completes.
/// \param hdl a handle to the target Task.
/// \return [Async] void
/// \ingroup Task
inline auto JoinTask(TaskHdl hdl) noexcept {
	return internal::JoinTaskOp{hdl};
}

/// \brief Alias for AkJoinTask
/// \param hdl a handle to the target Task.
/// \return [Async] void
/// \ingroup Task
inline auto operator co_await(TaskHdl hdl) noexcept {
	return internal::JoinTaskOp{hdl};
}

/// \brief Resturns the current TaskState.
/// \param hdl a handle to the target Task.
/// \return the current TaskState
/// \ingroup Task
inline TaskState GetTaskState(TaskHdl hdl) noexcept {
	return hdl.promise().state;
}

/// \brief Returns true if the target Task is done.
/// \param hdl a handle to the target Task
/// \return `true` if the target Task is done
/// \ingroup Task
inline bool IsTaskDone(TaskHdl hdl) noexcept {
	return hdl.done();
}

/// \brief Resumes a Task that is in TaskState::READY
/// \param hdl a handle to the target Task
/// \return true if the target Task is done
/// \ingroup Task
inline auto ResumeTask(TaskHdl hdl) noexcept {
	return internal::ResumeTaskOp{hdl};
}

/// \brief Configuration for the Kernel
/// \ingroup Kernel
struct KernelConfig {
    void*    mem;
    Size     memSize;
    unsigned ioEntryCount;
};

// Task::AwaitTaskEffect
// ----------------------------------------------------------------------------------------------------------------

namespace internal 
{
    
    inline static TaskContext* GetLinkedTaskContext(const DLink* link) noexcept {
        unsigned long long promise_off = ((unsigned long long)link) - offsetof(TaskContext, waitLink);
        return (TaskContext*)promise_off;
    }

    inline auto RunSchedulerTask() noexcept {
        struct RunSchedulerTaskOp {

            constexpr bool await_ready() const noexcept  { return false; }
            constexpr void await_resume() const noexcept {}  

            TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
                using namespace internal;

                (void)currentTaskHdl;
                TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();

                // Check expected state post scheduler construction

                assert(gKernel.taskCount == 1);
                assert(gKernel.readyCount == 1);
                assert(schedulerPromise.state == TaskState::READY);
                assert(!IsLinkDetached(&schedulerPromise.waitLink));
                assert(gKernel.currentTaskHdl == TaskHdl());

                // Setup SchedulerTask for execution (from READY -> RUNNING)
                gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
                schedulerPromise.state = TaskState::RUNNING;
                DetachLink(&schedulerPromise.waitLink);
                --gKernel.readyCount;

                // Check expected state post task system bootstrap
                CheckInvariants();
                return gKernel.schedulerTaskHdl;
            }        
        };

        return RunSchedulerTaskOp{};
    }

    inline auto TerminateSchedulerTask() noexcept {

        struct TerminateSchedulerOp {
            constexpr bool await_ready() const noexcept { return false; }
            KernelTaskHdl await_suspend(TaskHdl hdl) const noexcept {
                using namespace internal;

                assert(gKernel.currentTaskHdl == gKernel.schedulerTaskHdl);
                assert(gKernel.currentTaskHdl == hdl);

                TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();
                assert(schedulerPromise.state == TaskState::RUNNING);
                assert(IsLinkDetached(&schedulerPromise.waitLink));

                schedulerPromise.state = TaskState::ZOMBIE;
                ClearTaskHdl(&gKernel.currentTaskHdl);
                EnqueueLink(&gKernel.zombieList, &schedulerPromise.waitLink);
                ++gKernel.zombieCount;

                return gKernel.kernelTask;
            }
            constexpr void await_resume() const noexcept {}
        };
        
        return TerminateSchedulerOp{};
    }

    inline void DestroySchedulerTask(TaskHdl hdl) noexcept {
        using namespace internal;
        TaskContext* promise = &hdl.promise();

        // Remove from Task list
        DetachLink(&promise->taskListLink);
        --gKernel.taskCount;

        // Remove from Zombie List
        DetachLink(&promise->waitLink);
        --gKernel.zombieCount;

        promise->state = TaskState::DELETING; //TODO: double check
        hdl.destroy();
    }

    struct KernelTaskPromise {
        
        template <typename... Args>
        KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept {}
        
        void* operator new(std::size_t n) noexcept {
            void* mem = std::malloc(n);
            if (!mem) return nullptr;
            return mem;
        }

        void  operator delete(void* ptr, std::size_t sz) {
            (void)sz;
            std::free(ptr);
        }

        KernelTaskHdl  get_return_object() noexcept   { return KernelTaskHdl::from_promise(*this); }
        constexpr auto initial_suspend() noexcept     { return std::suspend_always {}; }
        constexpr auto final_suspend() noexcept       { return std::suspend_never  {}; }
        constexpr void return_void() noexcept         { }
        constexpr void unhandled_exception() noexcept { assert(false); }
    };

    struct DefineKernelTask {
        using promise_type = KernelTaskPromise;

        DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {} 
        operator KernelTaskHdl() const noexcept { return hdl; }

        KernelTaskHdl hdl;
    };

    /// \brief Schedules the next task
    /// 
    /// Used in Operations to schedule the next task.
    /// Assumes that the current task has been already suspended (moved to READY, WAITING, IO_WAITING, ...)
    ///
    /// \return the next Task to be resumed
    /// \internal
    inline TaskHdl ScheduleNextTask() noexcept {
        using namespace internal;

        // If we have a ready task, resume it
        while (true) {
            if (gKernel.readyCount > 0) {
                DLink* link = DequeueLink(&gKernel.readyList);
                TaskContext* ctx = GetLinkedTaskContext(link);
                TaskHdl task = TaskHdl::from_promise(*ctx);
                assert(ctx->state == TaskState::READY);
                ctx->state = TaskState::RUNNING;
                --gKernel.readyCount;
                gKernel.currentTaskHdl = task;
                CheckInvariants();
                return task;
            }

            if (gKernel.ioWaitingCount > 0) {
                unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
                // Submit Ready IO Operations
                if (ready > 0) {
                    int ret = io_uring_submit(&gKernel.ioRing);
                    if (ret < 0) {
                        std::print("io_uring_submit failed\n");
                        fflush(stdout);
                        abort();
                    }
                }

                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
                
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
                assert(zombiePromise.state == TaskState::ZOMBIE);

                // Remove from zombie list
                --gKernel.zombieCount;
                DetachLink(&zombiePromise.waitLink);

                // Remove from task list
                DetachLink(&zombiePromise.taskListLink);
                --gKernel.taskCount;

                // Delete
                zombiePromise.state = TaskState::DELETING;
                TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
                zombieTaskHdl.destroy();

                DebugTaskCount();
            }

            if (gKernel.readyCount == 0) {
                abort();
            }
        }
        // unreachable
        abort();
    }

    template <typename... Args>
    inline DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept {
        using namespace internal;

        TaskHdl mainTask = mainProc(args...);
        assert(!mainTask.done());
        assert(GetTaskState(mainTask) == TaskState::READY);

        while (true) {
            // Sumbit IO operations
            unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
            if (ready > 0) {
                int ret = io_uring_submit(&gKernel.ioRing);
                if (ret < 0) {
                    std::print("io_uring_submit failed\n");
                    fflush(stdout);
                    abort();
                }
            }

            // If we have a ready task, resume it
            if (gKernel.readyCount > 0) {
                DLink* nextNode = gKernel.readyList.prev;
                TaskContext* nextPromise = GetLinkedTaskContext(nextNode);
                TaskHdl nextTask = TaskHdl::from_promise(*nextPromise);
                assert(nextTask != gKernel.schedulerTaskHdl);
                co_await ResumeTaskOp(nextTask);
                assert(gKernel.currentTaskHdl);
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
                assert(zombiePromise.state == TaskState::ZOMBIE);

                // Remove from zombie list
                --gKernel.zombieCount;
                DetachLink(&zombiePromise.waitLink);

                // Remove from task list
                DetachLink(&zombiePromise.taskListLink);
                --gKernel.taskCount;

                // Delete
                zombiePromise.state = TaskState::DELETING;
                TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
                zombieTaskHdl.destroy();

                DebugTaskCount();
            }

            bool waitingCC = gKernel.ioWaitingCount;
            if (waitingCC) {
                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
            }

            if (gKernel.readyCount == 0 && gKernel.ioWaitingCount == 0) {
                break;
            }
        }
        co_await TerminateSchedulerTask();

        assert(false); // Unreachale
        co_return;
    }

    template <typename... Args>
    inline DefineKernelTask KernelTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args ... args) noexcept {

        TaskHdl schedulerHdl = SchedulerTaskProc(mainProc,  std::forward<Args>(args) ... );
        gKernel.schedulerTaskHdl = schedulerHdl;

        co_await RunSchedulerTask();
        DestroySchedulerTask(schedulerHdl);
        DebugTaskCount();

        co_return;
    }

    int InitAllocTable(void* mem, Size size) noexcept;

    inline int InitKernel(KernelConfig* config) noexcept {
        using namespace internal;
        
        if (InitAllocTable(config->mem, config->memSize) != 0) {
            return -1;
        }

        int res = io_uring_queue_init(config->ioEntryCount, &gKernel.ioRing, 0);
        if (res < 0) {
            std::print("io_uring_queue_init failed\n");
            return -1;
        }

        gKernel.mem = config->mem;
        gKernel.memSize = config->memSize;
        gKernel.taskCount = 0;
        gKernel.readyCount = 0;
        gKernel.waitingCount = 0;
        gKernel.ioWaitingCount = 0;
        gKernel.zombieCount = 0;
        gKernel.interrupted = 0;

        ClearTaskHdl(&gKernel.currentTaskHdl);
        ClearTaskHdl(&gKernel.schedulerTaskHdl);

        InitLink(&gKernel.zombieList);
        InitLink(&gKernel.readyList);
        InitLink(&gKernel.taskList);
        
        return 0;
    }

    inline void FiniKernel() noexcept {
        io_uring_queue_exit(&gKernel.ioRing);
    }

    inline void InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        TaskContext* promise = &hdl.promise();

        // Check initial preconditions
        assert(promise->state == TaskState::CREATED);
        assert(IsLinkDetached(&promise->waitLink));
        CheckInvariants();

        // Add task to the kernel
        ++gKernel.taskCount;
        EnqueueLink(&gKernel.taskList, &promise->taskListLink);

        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &promise->waitLink);
        promise->state = TaskState::READY;

        // Check post-conditions
        assert(promise->state == TaskState::READY);
        assert(!IsLinkDetached(&promise->waitLink));
        CheckInvariants();
        internal::DebugTaskCount();
    }

    inline TaskHdl FinalSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        // Check preconditions
        TaskContext* ctx = &hdl.promise();
        assert(gKernel.currentTaskHdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&ctx->waitLink));
        CheckInvariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = TaskState::ZOMBIE;
        ++gKernel.zombieCount;
        EnqueueLink(&gKernel.zombieList, &ctx->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    inline void DebugTaskCount() noexcept {
        if constexpr (TRACE_DEBUG_CODE) {
            int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
            std::print("----------------:--------\n");
            std::print("Task       count: {}\n", gKernel.taskCount);
            std::print("----------------:--------\n");
            std::print("Running    count: {}\n", running_count);
            std::print("Ready      count: {}\n", gKernel.readyCount);
            std::print("Waiting    count: {}\n", gKernel.waitingCount);
            std::print("IO waiting count: {}\n", gKernel.ioWaitingCount);
            std::print("Zombie     count: {}\n", gKernel.zombieCount);
            std::print("----------------:--------\n");
        }
    }

    inline void DoCheckTaskCountInvariant() noexcept {
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
        bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount;
        if (!condition) {
            DebugTaskCount();
            abort();
        }
    }

    inline void CheckTaskCountInvariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline void CheckInvariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }

    inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept
    {
        TaskContext* currentTaskCtx = &currentTaskHdl.promise();

        // Check CurrentTask preconditions
        assert(currentTaskCtx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentTaskCtx->waitLink));
        assert(gKernel.currentTaskHdl == currentTaskHdl);
        CheckInvariants();

        TaskContext* joinedTaskCtx = &joinedTaskHdl.promise();                
        TaskState joinedTaskState = joinedTaskCtx->state;
        switch (joinedTaskState) {
            case TaskState::READY:
            {

                // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the joined TASK from READY to RUNNING
                joinedTaskCtx->state = TaskState::RUNNING;
                DetachLink(&joinedTaskCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = joinedTaskHdl;
                CheckInvariants();
                DebugTaskCount();
                return joinedTaskHdl;
            }

            case TaskState::IO_WAITING:
            case TaskState::WAITING:
            {
                 // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the Scheduler Task from READY to RUNNING
                TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
                assert(schedCtx->state == TaskState::READY);
                schedCtx->state = TaskState::RUNNING;
                DetachLink(&schedCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
                CheckInvariants();
                DebugTaskCount();

                return gKernel.schedulerTaskHdl;
            }
            
            case TaskState::DELETING:
            case TaskState::ZOMBIE:
            {
                return currentTaskHdl;
            }
            
            case TaskState::INVALID:
            case TaskState::CREATED:
            case TaskState::RUNNING:
            default:
            {
                // Illegal State
                abort();
            }
        }
    }

    inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
        assert(gKernel.currentTaskHdl);

        TaskContext* currentPromise = &currentTask.promise();

        if constexpr (IS_DEBUG_MODE) {
            assert(gKernel.currentTaskHdl == currentTask);
            assert(currentPromise->state == TaskState::RUNNING);
            assert(IsLinkDetached(&currentPromise->waitLink));
            CheckInvariants();
        }

        // Move the current task from RUNNINIG to READY
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
        assert(gKernel.currentTaskHdl == currentTaskHdl);

        // Check the current Task
        TaskContext* currentPromise = &gKernel.currentTaskHdl.promise();
        assert(IsLinkDetached(&currentPromise->waitLink));
        assert(currentPromise->state == TaskState::RUNNING);
        CheckInvariants();

        // Suspend the current Task
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        // Move the target task from READY to RUNNING
        TaskContext* promise = &hdl.promise();
        promise->state = TaskState::RUNNING;
        DetachLink(&promise->waitLink);
        --gKernel.readyCount;
        gKernel.currentTaskHdl = hdl;
        CheckInvariants();

        assert(gKernel.currentTaskHdl);
        return hdl;
    }




}

/// \brief Runs the main task
/// \param UserProc the user's main task
/// \return 0 on success
/// \ingroup Kernel
template <typename... Args>
inline int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept {  
    using namespace internal;

    if (InitKernel(config) < 0) {
        return -1;
    }

    KernelTaskHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
    gKernel.kernelTask = hdl;
    hdl.resume();

    FiniKernel();

    return 0;
}

inline void TaskContext::return_void() noexcept {
    using namespace internal;

    CheckInvariants();

    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&awaitingTerminationList)) {
        return;
    }

    do {
        Link* next = DequeueLink(&awaitingTerminationList);
        TaskContext* ctx = GetLinkedTaskContext(next);
        DebugTaskCount();
        assert(ctx->state == TaskState::WAITING);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        DebugTaskCount();

    } while (!IsLinkDetached(&awaitingTerminationList));
}

namespace internal {
#ifdef AK_IMPLEMENTATION    
    alignas(64) struct Kernel gKernel;
#endif
}

struct Event {  
    internal::DLink waitingList;
};

inline void InitEvent(Event* event) {
    InitLink(&event->waitingList);
}

inline int SignalOne(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    
    if (IsLinkDetached(&event->waitingList)) return 0;

    DLink* link = DequeueLink(&event->waitingList);
    TaskContext* ctx = GetLinkedTaskContext(link);
    assert(ctx->state == TaskState::WAITING);
    
    // Move the target task from WAITING to READY
    DetachLink(link);
    --gKernel.waitingCount;
    ctx->state = TaskState::READY;
    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
    ++gKernel.readyCount;
    return 1;
}

inline int SignalSome(Event* event, int n) {
    using namespace internal;
    assert(event != nullptr);
    assert(n >= 0);
    int cc = 0;
    while (cc < n && !IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;    
        ++cc;
    }
    return cc;
}

inline int SignalAll(Event* event) {
    using namespace internal;
    assert(event != nullptr);
    int signalled = 0;
    while (!IsLinkDetached(&event->waitingList)) {
        DLink* link = DequeueLink(&event->waitingList);
        TaskContext* ctx = GetLinkedTaskContext(link);
        assert(ctx->state == TaskState::WAITING);
        
        // Move the target task from WAITING to READY
        DetachLink(link);
        --gKernel.waitingCount;
        ctx->state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &ctx->waitLink);
        ++gKernel.readyCount;
        
        ++signalled;        
    }
    return signalled;
}

inline auto WaitEvent(Event* event) {
    using namespace internal;
    
    assert(event != nullptr);
    
    struct WaitOp {
        
        WaitOp(Event* event) : evt(event) {}

        constexpr bool await_ready() const noexcept { 
            return false; 
        }

        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept {
            using namespace internal;

            TaskContext* ctx = &hdl.promise();
            assert(gKernel.currentTaskHdl == hdl);
            assert(ctx->state == TaskState::RUNNING);
            
            // Move state from RUNNING to WAITING  
            ctx->state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&evt->waitingList, &ctx->waitLink);
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();

            return ScheduleNextTask();
        }

        constexpr void await_resume() const noexcept { }

        Event* evt;
    };

    return WaitOp{event};
}

// -----------------------------------------------------------------------------
// DebugAPI 
// -----------------------------------------------------------------------------

inline void DebugIOURingFeatures(const unsigned int features) {
    std::print("IO uring features:\n");
    if (features & IORING_FEAT_SINGLE_MMAP)     std::print("  SINGLE_MMAP\n");
    if (features & IORING_FEAT_NODROP)          std::print("  NODROP\n");
    if (features & IORING_FEAT_SUBMIT_STABLE)   std::print("  SUBMIT_STABLE\n");
    if (features & IORING_FEAT_RW_CUR_POS)      std::print("  RW_CUR_POS\n");
    if (features & IORING_FEAT_CUR_PERSONALITY) std::print("  CUR_PERSONALITY\n");
    if (features & IORING_FEAT_FAST_POLL)       std::print("  FAST_POLL\n");
    if (features & IORING_FEAT_POLL_32BITS)     std::print("  POLL_32BITS\n");
    if (features & IORING_FEAT_SQPOLL_NONFIXED) std::print("  SQPOLL_NONFIXED\n");
    if (features & IORING_FEAT_EXT_ARG)         std::print("  EXT_ARG\n");
    if (features & IORING_FEAT_NATIVE_WORKERS)  std::print("  NATIVE_WORKERS\n");
}

inline void DebugIOURingSetupFlags(const unsigned int flags) {
    std::print("IO uring flags:\n");
    if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
    if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
    if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
    if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
    if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
    if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
}

inline void DebugIOURingParams(const io_uring_params* p) {
    std::print("IO uring parameters:\n");
    
    // Main parameters
    std::print("Main Configuration:\n");
    std::print("  sq_entries: {}\n", p->sq_entries);
    std::print("  cq_entries: {}\n", p->cq_entries);
    std::print("  sq_thread_cpu: {}\n", p->sq_thread_cpu);
    std::print("  sq_thread_idle: {}\n", p->sq_thread_idle);
    std::print("  wq_fd: {}\n", p->wq_fd);

    // Print flags
    DebugIOURingSetupFlags(p->flags);

    // Print features
    DebugIOURingFeatures(p->features);

    // Submission Queue Offsets

    std::print("Submission Queue Offsets:\n");
    std::print("  head: {}\n", p->sq_off.head);
    std::print("  tail: {}\n", p->sq_off.tail);
    std::print("  ring_mask: {}\n", p->sq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->sq_off.ring_entries);
    std::print("  flags: {}\n", p->sq_off.flags);
    std::print("  dropped: {}\n", p->sq_off.dropped);
    std::print("  array: {}\n", p->sq_off.array);

    // Completion Queue Offsets

    std::print("Completion Queue Offsets:\n");
    std::print("  head: {}\n", p->cq_off.head);
    std::print("  tail: {}\n", p->cq_off.tail);
    std::print("  ring_mask: {}\n", p->cq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->cq_off.ring_entries);
    std::print("  overflow: {}\n", p->cq_off.overflow);
    std::print("  cqes: {}\n", p->cq_off.cqes);
    std::print("  flags: {}\n", p->cq_off.flags);
    std::print("\n");
    std::fflush(stdout);
}

// -----------------------------------------------------------------------------
// IO Operators
// -----------------------------------------------------------------------------

inline int GetCurrentTaskEnqueuedIOOps() noexcept {
    return GetTaskContext()->enqueuedIO;
}

namespace internal {
    
    // todo
    struct IODrainOp {
        constexpr bool await_ready() const noexcept { 
            // todo: 
            return false;
        }
        
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept {
            // todo: 
            return currentTaskHdl;
        }

        constexpr int await_resume() const noexcept { 
            // todo: 
            return 0;
        }
    };

    struct IOWaitOneOp {
        constexpr bool await_ready() const noexcept { 
            return false;
        }
        
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept {
            // if suspend is called we know that the operation has been submitted
            using namespace internal;

            // Move the current Task from RUNNING to IO_WAITING
            TaskContext* ctx = &currentTaskHdl.promise();
            assert(ctx->state == TaskState::RUNNING);
            ctx->state = TaskState::IO_WAITING;
            ++gKernel.ioWaitingCount;
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();
            DebugTaskCount();

            // Move the scheduler task from READY to RUNNING
            TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
            assert(schedCtx->state == TaskState::READY);
            schedCtx->state = TaskState::RUNNING;
            DetachLink(&schedCtx->waitLink);
            --gKernel.readyCount;
            gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
            CheckInvariants();
            DebugTaskCount();

            return gKernel.schedulerTaskHdl;
        }

        constexpr int await_resume() const noexcept { return gKernel.currentTaskHdl.promise().ioResult; }
    };

    template <typename Prep>
    inline internal::IOWaitOneOp PrepareIO(Prep prep) noexcept {
        using namespace internal;
        
        TaskContext* ctx = &gKernel.currentTaskHdl.promise();

        // Ensure free submission slot 
        unsigned int free_slots = io_uring_sq_space_left(&gKernel.ioRing);
        while (free_slots < 1) {
            int ret = io_uring_submit(&gKernel.ioRing);
            if (ret < 0) {
                abort();
                // unreachable
            }
            free_slots = io_uring_sq_space_left(&gKernel.ioRing);
        }

        // Enqueue operation
        io_uring_sqe* sqe = io_uring_get_sqe(&gKernel.ioRing);
        io_uring_sqe_set_data(sqe, (void*) ctx);
        prep(sqe);  // Call the preparation function
        ctx->ioResult = 0;
        ++ctx->enqueuedIO;
        return {};
    }
  
}


// File Operations
inline internal::IOWaitOneOp IOOpen(const char* path, int flags, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);
    });
}

inline internal::IOWaitOneOp IOOpenAt(int dfd, const char* path, int flags, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, dfd, path, flags, mode);
    });
}

inline internal::IOWaitOneOp IOOpenAtDirect(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat_direct(sqe, dfd, path, flags, mode, file_index);
    });
}

inline internal::IOWaitOneOp IOClose(int fd) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_close(sqe, fd);
    });
}

inline internal::IOWaitOneOp IOCloseDirect(unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_close_direct(sqe, file_index);
    });
}

// Read Operations
inline internal::IOWaitOneOp IORead(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    });
}

inline internal::IOWaitOneOp IOReadMultishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read_multishot(sqe, fd, nbytes, offset, buf_group);
    });
}

inline internal::IOWaitOneOp IOReadFixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

inline internal::IOWaitOneOp IOReadV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
    });
}

inline internal::IOWaitOneOp IOReadV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

inline internal::IOWaitOneOp IOReadVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_readv_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Write Operations
inline internal::IOWaitOneOp IOWrite(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    });
}

inline internal::IOWaitOneOp IOWriteFixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

inline internal::IOWaitOneOp IOWriteV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
    });
}

inline internal::IOWaitOneOp IOWriteV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

inline internal::IOWaitOneOp IOWriteVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_writev_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Socket Operations
inline internal::IOWaitOneOp IOAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOWaitOneOp IOAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_accept_direct(sqe, fd, addr, addrlen, flags, file_index);
    });
}

inline internal::IOWaitOneOp IOMultishotAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOWaitOneOp IOMultishotAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept_direct(sqe, fd, addr, addrlen, flags);
    });
}

inline internal::IOWaitOneOp IOConnect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrlen);
    });
}

inline internal::IOWaitOneOp IOSend(int sockfd, const void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOWaitOneOp IOSendZC(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc(sqe, sockfd, buf, len, flags, zc_flags);
    });
}

inline internal::IOWaitOneOp IOSendZCFixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, zc_flags, buf_index);
    });
}

inline internal::IOWaitOneOp IOSendMsg(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg(sqe, fd, msg, flags);
    });
}

inline internal::IOWaitOneOp IOSendMsgZC(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc(sqe, fd, msg, flags);
    });
}

inline internal::IOWaitOneOp IOSendMsgZCFixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc_fixed(sqe, fd, msg, flags, buf_index);
    });
}

inline internal::IOWaitOneOp IORecv(int sockfd, void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOWaitOneOp IORecvMultishot(int sockfd, void* buf, size_t len, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recv_multishot(sqe, sockfd, buf, len, flags);
    });
}

inline internal::IOWaitOneOp IORecvMsg(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, msg, flags);
    });
}

inline internal::IOWaitOneOp IORecvMsgMultishot(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg_multishot(sqe, fd, msg, flags);
    });
}

inline internal::IOWaitOneOp IOSocket(int domain, int type, int protocol, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_socket(sqe, domain, type, protocol, flags);
    });
}

inline internal::IOWaitOneOp IOSocketDirect(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_socket_direct(sqe, domain, type, protocol, file_index, flags);
    });
}

// Directory and Link Operations
inline internal::IOWaitOneOp IOMkdir(const char* path, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdir(sqe, path, mode);
    });
}

inline internal::IOWaitOneOp IOMkdirAt(int dfd, const char* path, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdirat(sqe, dfd, path, mode);
    });
}

inline internal::IOWaitOneOp IOSymlink(const char* target, const char* linkpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_symlink(sqe, target, linkpath);
    });
}

inline internal::IOWaitOneOp IOSymlinkAt(const char* target, int newdirfd, const char* linkpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_symlinkat(sqe, target, newdirfd, linkpath);
    });
}

inline internal::IOWaitOneOp IOLink(const char* oldpath, const char* newpath, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_link(sqe, oldpath, newpath, flags);
    });
}

inline internal::IOWaitOneOp IOLinkAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_linkat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}

// File Management Operations
inline internal::IOWaitOneOp IOUnlink(const char* path, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_unlink(sqe, path, flags);
    });
}

inline internal::IOWaitOneOp IOUnlinkAt(int dfd, const char* path, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_unlinkat(sqe, dfd, path, flags);
    });
}

inline internal::IOWaitOneOp IORename(const char* oldpath, const char* newpath) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_rename(sqe, oldpath, newpath);
    });
}

inline internal::IOWaitOneOp IORenameAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_renameat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}

inline internal::IOWaitOneOp IOSync(int fd, unsigned fsync_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fsync(sqe, fd, fsync_flags);
    });
}

inline internal::IOWaitOneOp IOSyncFileRange(int fd, unsigned len, __u64 offset, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_sync_file_range(sqe, fd, len, offset, flags);
    });
}

inline internal::IOWaitOneOp IOFAllocate(int fd, int mode, __u64 offset, __u64 len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fallocate(sqe, fd, mode, offset, len);
    });
}

inline internal::IOWaitOneOp IOOpenAt2(int dfd, const char* path, struct open_how* how) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2(sqe, dfd, path, how);
    });
}

inline internal::IOWaitOneOp IOOpenAt2Direct(int dfd, const char* path, struct open_how* how, unsigned file_index) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2_direct(sqe, dfd, path, how, file_index);
    });
}

inline internal::IOWaitOneOp IOStatx(int dfd, const char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
    });
}

inline internal::IOWaitOneOp IOFAdvise(int fd, __u64 offset, __u32 len, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise(sqe, fd, offset, len, advice);
    });
}

inline internal::IOWaitOneOp IOFAdvise64(int fd, __u64 offset, off_t len, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise64(sqe, fd, offset, len, advice);
    });
}

inline internal::IOWaitOneOp IOMAdvise(void* addr, __u32 length, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise(sqe, addr, length, advice);
    });
}

inline internal::IOWaitOneOp IOMAdvise64(void* addr, off_t length, int advice) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise64(sqe, addr, length, advice);
    });
}

// Extended Attributes Operations
inline internal::IOWaitOneOp IOGetXAttr(const char* name, char* value, const char* path, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_getxattr(sqe, name, value, path, len);
    });
}

inline internal::IOWaitOneOp IOSetXAttr(const char* name, const char* value, const char* path, int flags, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_setxattr(sqe, name, value, path, flags, len);
    });
}

inline internal::IOWaitOneOp IOFGetXAttr(int fd, const char* name, char* value, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fgetxattr(sqe, fd, name, value, len);
    });
}

inline internal::IOWaitOneOp IOFSetXAttr(int fd, const char* name, const char* value, int flags, unsigned int len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fsetxattr(sqe, fd, name, value, flags, len);
    });
}

// Buffer Operations
inline internal::IOWaitOneOp IOProvideBuffers(void* addr, int len, int nr, int bgid, int bid) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
    });
}

inline internal::IOWaitOneOp IORemoveBuffers(int nr, int bgid) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_remove_buffers(sqe, nr, bgid);
    });
}

// Polling Operations
inline internal::IOWaitOneOp IOPollAdd(int fd, unsigned poll_mask) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_add(sqe, fd, poll_mask);
    });
}

inline internal::IOWaitOneOp IOPollMultishot(int fd, unsigned poll_mask) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_multishot(sqe, fd, poll_mask);
    });
}

inline internal::IOWaitOneOp IOPollRemove(__u64 user_data) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_remove(sqe, user_data);
    });
}

inline internal::IOWaitOneOp IOPollUpdate(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_update(sqe, old_user_data, new_user_data, poll_mask, flags);
    });
}

inline internal::IOWaitOneOp IOEpollCtl(int epfd, int fd, int op, struct epoll_event* ev) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_ctl(sqe, epfd, fd, op, ev);
    });
}

inline internal::IOWaitOneOp IOEpollWait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_wait(sqe, fd, events, maxevents, flags);
    });
}

// Timeout Operations
inline internal::IOWaitOneOp IOTimeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout(sqe, ts, count, flags);
    });
}

inline internal::IOWaitOneOp IOTimeoutRemove(__u64 user_data, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_remove(sqe, user_data, flags);
    });
}

inline internal::IOWaitOneOp IOTimeoutUpdate(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_update(sqe, ts, user_data, flags);
    });
}

inline internal::IOWaitOneOp IOLinkTimeout(struct __kernel_timespec* ts, unsigned flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_link_timeout(sqe, ts, flags);
    });
}

// Message Ring Operations
inline internal::IOWaitOneOp IOMsgRing(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring(sqe, fd, len, data, flags);
    });
}

inline internal::IOWaitOneOp IOMsgRingCqeFlags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_cqe_flags(sqe, fd, len, data, flags, cqe_flags);
    });
}

inline internal::IOWaitOneOp IOMsgRingFd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd(sqe, fd, source_fd, target_fd, data, flags);
    });
}

inline internal::IOWaitOneOp IOMsgRingFdAlloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd_alloc(sqe, fd, source_fd, data, flags);
    });
}

// Process Operations
inline internal::IOWaitOneOp IOWaitId(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_waitid(sqe, idtype, id, infop, options, flags);
    });
}

// Futex Operations
inline internal::IOWaitOneOp IOFutexWake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wake(sqe, futex, val, mask, futex_flags, flags);
    });
}

inline internal::IOWaitOneOp IOFutexWait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wait(sqe, futex, val, mask, futex_flags, flags);
    });
}

inline internal::IOWaitOneOp IOFutexWaitV(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_waitv(sqe, futex, nr_futex, flags);
    });
}

// File Descriptor Management
inline internal::IOWaitOneOp IOFixedFdInstall(int fd, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_fixed_fd_install(sqe, fd, flags);
    });
}

inline internal::IOWaitOneOp IOFilesUpdate(int* fds, unsigned nr_fds, int offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_files_update(sqe, fds, nr_fds, offset);
    });
}

// Shutdown Operation
inline internal::IOWaitOneOp IOShutdown(int fd, int how) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_shutdown(sqe, fd, how);
    });
}

// File Truncation
inline internal::IOWaitOneOp IOFTruncate(int fd, loff_t len) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_ftruncate(sqe, fd, len);
    });
}

// Command Operations
inline internal::IOWaitOneOp IOCmdSock(int cmd_op, int fd, int level, int optname, void* optval, int optlen) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_sock(sqe, cmd_op, fd, level, optname, optval, optlen);
    });
}

inline internal::IOWaitOneOp IOCmdDiscard(int fd, uint64_t offset, uint64_t nbytes) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_discard(sqe, fd, offset, nbytes);
    });
}

// Special Operations
inline internal::IOWaitOneOp IONop() noexcept {
    return internal::PrepareIO([](io_uring_sqe* sqe) {
        io_uring_prep_nop(sqe);
    });
}

// Splice Operations
inline internal::IOWaitOneOp IOSplice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
    });
}

inline internal::IOWaitOneOp IOTee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_tee(sqe, fd_in, fd_out, nbytes, splice_flags);
    });
}

// Cancel Operations
inline internal::IOWaitOneOp IOCancel64(__u64 user_data, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel64(sqe, user_data, flags);
    });
}

inline internal::IOWaitOneOp IOCancel(void* user_data, int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel(sqe, user_data, flags);
    });
}

inline internal::IOWaitOneOp IOCancelFd(int fd, unsigned int flags) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel_fd(sqe, fd, flags);
    });
}

inline void DebugDumpAllocTable() noexcept {
    using namespace internal;

    AllocTable* at = (AllocTable*)&gKernel.allocTable;

    // Basic layout and sizes
    std::print("AllocTable: {}\n", (void*)at);
    
    std::print("  heapBegin        : {}\n", (void*)at->heapBegin);
    std::print("  heapEnd          : {}; size: {}\n", (void*)at->heapEnd, (intptr_t)(at->heapEnd - at->heapBegin));
    std::print("  memBegin         : {}\n", (void*)at->memBegin);
    std::print("  memEnd           : {}; size: {}\n", (void*)at->memEnd, (intptr_t)(at->memEnd - at->memBegin));
    std::print("  memSize          : {}\n", at->memSize);
    std::print("  usedMemSize      : {}\n", at->usedMemSize);
    std::print("  freeMemSize      : {}\n", at->freeMemSize);

    // Sentinels and wild/large tracking (addresses only; do not dereference)
    std::print("  Key Offsets:\n");
    std::print("    Begin sentinel offset: {}\n", (intptr_t)at->beginSentinel - (intptr_t)at->memBegin);
    std::print("    Wild  block    offset: {}\n", (intptr_t)at->wildBlock - (intptr_t)at->memBegin);
    std::print("    LB    sentinel offset: {}\n", (intptr_t)at->largeBlockSentinel - (intptr_t)at->memBegin);
    std::print("    End   sentinel offset: {}\n", (intptr_t)at->endSentinel - (intptr_t)at->memBegin);

    // Free list availability mask as a bit array (256 bits)
    std::print("  FreeListbinMask:");
    for (unsigned i = 0; i < 256; i++) {
        if (i % 64 == 0) std::print("\n    ");
        std::print("{}", (at->freeListbinMask[i / 32] >> (i % 32)) & 1u);
    }
        std::print("\n");

    // Optional per-bin size accounting
    
    std::print("  FreeListBinsSizes begin\n");
    for (unsigned i = 0; i < 254; ++i) {
        unsigned cc = at->freeListBinsCount[i];
        if (cc == 0) continue;
        std::print("    {:>3} bytes class  : {}\n", i * 32, cc);
    }
    std::print("    medium class (254) : {}\n", at->freeListBinsCount[254]);
    std::print("    wild class   (255) : {}\n", at->freeListBinsCount[255]);
    std::print("  FreeListBinsSizes end\n");
    

    // Aggregate statistics
    // std::print("maxFreeBlockSize: {}\n", at->maxFreeBlockSize);
    // std::print("totalAllocCount: {}\n", at->totalAllocCount);
    // std::print("totalFreeCount: {}\n", at->totalFreeCount);
    // std::print("totalReallocCount: {}\n", at->totalReallocCount);
    // std::print("totalSplitCount: {}\n", at->totalSplitCount);
    // std::print("totalMergeCount: {}\n", at->totalMergeCount);
    // std::print("totalReuseCount: {}\n", at->totalReuseCount);
            
    std::print("\n");
}

namespace internal {

    static inline constexpr void SetAllocFreeBinBit(AllocTable* at, unsigned binIdx) {       
        assert(at != nullptr);
        assert(binIdx < 256);
        at->freeListbinMask[binIdx >> 5] |= 1u << (binIdx & 31u);
    }

    static inline constexpr bool GetAllocFreeBinBit(AllocTable* at, unsigned binIdx) {       
        assert(at != nullptr);
        assert(binIdx < 256);
        return (at->freeListbinMask[binIdx >> 5] >> (binIdx & 31u)) & 1u;
    }

    inline int InitAllocTable(void* mem, Size size) noexcept {
        
        
        constexpr U64 SENTINEL_SIZE = sizeof(FreeAllocHeader);

        assert(mem != nullptr);
        assert(size >= 4096);

        AllocTable* at = (AllocTable*)&gKernel.allocTable;
        memset((void*)at, 0, sizeof(AllocTable));
        

        // Establish heap boundaries
        char* heapBegin = (char*)(mem);
        char* heapEnd   = heapBegin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 alignedBegin = ((U64)heapBegin + SENTINEL_SIZE) & ~31ull;
        U64 alignedEnd   = ((U64)heapEnd   - SENTINEL_SIZE) & ~31ull;

        at->heapBegin = heapBegin;
        at->heapEnd   = heapEnd;
        at->memBegin  = (char*)alignedBegin;
        at->memEnd    = (char*)alignedEnd;
        at->memSize   = (Size)(at->memEnd - at->memBegin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocHeader* beginSentinel      = (FreeAllocHeader*)alignedBegin;
        FreeAllocHeader* wildBlock          = (FreeAllocHeader*)((char*)beginSentinel + SENTINEL_SIZE);
        FreeAllocHeader* endSentinel        = (FreeAllocHeader*)((char*)alignedEnd - SENTINEL_SIZE); 
        FreeAllocHeader* largeBlockSentinel = (FreeAllocHeader*)((char*)endSentinel - SENTINEL_SIZE);
        InitLink(&wildBlock->freeListLink);
        
        // Check alignments
        assert(((U64)beginSentinel      & 31ull) == 0ull);
        assert(((U64)wildBlock          & 31ull) == 0ull);
        assert(((U64)endSentinel        & 31ull) == 0ull);
        assert(((U64)largeBlockSentinel & 31ull) == 0ull);
        
        at->beginSentinel      = beginSentinel;
        at->wildBlock          = wildBlock;
        at->endSentinel        = endSentinel;
        at->largeBlockSentinel = largeBlockSentinel;
        
        beginSentinel->thisSize.size       = (U64)SENTINEL_SIZE;
        beginSentinel->thisSize.state      = (U32)AllocState::BEGIN_SENTINEL;
        wildBlock->thisSize.size           = (U64)((U64)largeBlockSentinel - (U64)wildBlock);
        wildBlock->thisSize.state          = (U32)AllocState::WILD_BLOCK;
        largeBlockSentinel->thisSize.size  = (U64)SENTINEL_SIZE;
        largeBlockSentinel->thisSize.state = (U32)AllocState::LARGE_BLOCK_SENTINEL;
        endSentinel->thisSize.size         = (U64)SENTINEL_SIZE;
        endSentinel->thisSize.state        = (U32)AllocState::END_SENTINEL;
        wildBlock->prevSize                = beginSentinel->thisSize;
        largeBlockSentinel->prevSize       = wildBlock->thisSize;
        endSentinel->prevSize              = largeBlockSentinel->thisSize;
        at->freeMemSize                    = wildBlock->thisSize.size;

        for (int i = 0; i < 256; ++i) {
            InitLink(&at->freeListBins[i]);
        }

        SetAllocFreeBinBit(at, 255);
        DLink* freeList = &at->freeListBins[255];
        InitLink(&wildBlock->freeListLink);
        InsertNextLink(freeList, &wildBlock->freeListLink);
        at->freeListBinsCount[255] = 1;

        return 0;
    }

}

namespace internal {
    constexpr const char* DEBUG_ALLOC_COLOR_RESET  = "\033[0m";
    constexpr const char* DEBUG_ALLOC_COLOR_WHITE  = "\033[37m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_GREEN  = "\033[1;32m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_YELLOW = "\033[1;33m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_CYAN   = "\033[36m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_MAG    = "\033[35m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_RED    = "\033[1;31m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_HDR    = "\033[36m"; 

    constexpr const char* DEBUG_FREELIST_HEAD_TABLE[] = {
        "HEAD(32)", "HEAD(64)", "HEAD(96)", "HEAD(128)", "HEAD(160)", "HEAD(192)", "HEAD(224)", "HEAD(256)", "HEAD(288)", "HEAD(320)", "HEAD(352)", "HEAD(384)", "HEAD(416)", "HEAD(448)", "HEAD(480)", "HEAD(512)", "HEAD(544)", "HEAD(576)", "HEAD(608)", "HEAD(640)", "HEAD(672)", "HEAD(704)", "HEAD(736)", "HEAD(768)", "HEAD(800)", "HEAD(832)", "HEAD(864)", "HEAD(896)", "HEAD(928)", "HEAD(960)", "HEAD(992)", "HEAD(1024)", "HEAD(1056)", "HEAD(1088)", "HEAD(1120)", "HEAD(1152)", "HEAD(1184)", "HEAD(1216)", "HEAD(1248)", "HEAD(1280)", "HEAD(1312)", "HEAD(1344)", "HEAD(1376)", "HEAD(1408)", "HEAD(1440)", "HEAD(1472)", "HEAD(1504)", "HEAD(1536)", "HEAD(1568)", "HEAD(1600)", "HEAD(1632)", "HEAD(1664)", "HEAD(1696)", "HEAD(1728)", "HEAD(1760)", "HEAD(1792)", "HEAD(1824)", "HEAD(1856)", "HEAD(1888)", "HEAD(1920)", "HEAD(1952)", "HEAD(1984)", "HEAD(2016)", "HEAD(2048)", "HEAD(2080)", "HEAD(2112)", "HEAD(2144)", "HEAD(2176)", "HEAD(2208)", "HEAD(2240)", "HEAD(2272)", "HEAD(2304)", "HEAD(2336)", "HEAD(2368)", "HEAD(2400)", "HEAD(2432)", "HEAD(2464)", "HEAD(2496)", "HEAD(2528)", "HEAD(2560)", "HEAD(2592)", "HEAD(2624)", "HEAD(2656)", "HEAD(2688)", "HEAD(2720)", "HEAD(2752)", "HEAD(2784)", "HEAD(2816)", "HEAD(2848)", "HEAD(2880)", "HEAD(2912)", "HEAD(2944)", "HEAD(2976)", "HEAD(3008)", "HEAD(3040)", "HEAD(3072)", "HEAD(3104)", "HEAD(3136)", "HEAD(3168)", "HEAD(3200)", "HEAD(3232)", "HEAD(3264)", "HEAD(3296)", "HEAD(3328)", "HEAD(3360)", "HEAD(3392)", "HEAD(3424)", "HEAD(3456)", "HEAD(3488)", "HEAD(3520)", "HEAD(3552)", "HEAD(3584)", "HEAD(3616)", "HEAD(3648)", "HEAD(3680)", "HEAD(3712)", "HEAD(3744)", "HEAD(3776)", "HEAD(3808)", "HEAD(3840)", "HEAD(3872)", "HEAD(3904)", "HEAD(3936)", "HEAD(3968)", "HEAD(4000)", "HEAD(4032)", "HEAD(4064)", "HEAD(4096)", "HEAD(4128)", "HEAD(4160)", "HEAD(4192)", "HEAD(4224)", "HEAD(4256)", "HEAD(4288)", "HEAD(4320)", "HEAD(4352)", "HEAD(4384)", "HEAD(4416)", "HEAD(4448)", "HEAD(4480)", "HEAD(4512)", "HEAD(4544)", "HEAD(4576)", "HEAD(4608)", "HEAD(4640)", "HEAD(4672)", "HEAD(4704)", "HEAD(4736)", "HEAD(4768)", "HEAD(4800)", "HEAD(4832)", "HEAD(4864)", "HEAD(4896)", "HEAD(4928)", "HEAD(4960)", "HEAD(4992)", "HEAD(5024)", "HEAD(5056)", "HEAD(5088)", "HEAD(5120)", "HEAD(5152)", "HEAD(5184)", "HEAD(5216)", "HEAD(5248)", "HEAD(5280)", "HEAD(5312)", "HEAD(5344)", "HEAD(5376)", "HEAD(5408)", "HEAD(5440)", "HEAD(5472)", "HEAD(5504)", "HEAD(5536)", "HEAD(5568)", "HEAD(5600)", "HEAD(5632)", "HEAD(5664)", "HEAD(5696)", "HEAD(5728)", "HEAD(5760)", "HEAD(5792)", "HEAD(5824)", "HEAD(5856)", "HEAD(5888)", "HEAD(5920)", "HEAD(5952)", "HEAD(5984)", "HEAD(6016)", "HEAD(6048)", "HEAD(6080)", "HEAD(6112)", "HEAD(6144)", "HEAD(6176)", "HEAD(6208)", "HEAD(6240)", "HEAD(6272)", "HEAD(6304)", "HEAD(6336)", "HEAD(6368)", "HEAD(6400)", "HEAD(6432)", "HEAD(6464)", "HEAD(6496)", "HEAD(6528)", "HEAD(6560)", "HEAD(6592)", "HEAD(6624)", "HEAD(6656)", "HEAD(6688)", "HEAD(6720)", "HEAD(6752)", "HEAD(6784)", "HEAD(6816)", "HEAD(6848)", "HEAD(6880)", "HEAD(6912)", "HEAD(6944)", "HEAD(6976)", "HEAD(7008)", "HEAD(7040)", "HEAD(7072)", "HEAD(7104)", "HEAD(7136)", "HEAD(7168)", "HEAD(7200)", "HEAD(7232)", "HEAD(7264)", "HEAD(7296)", "HEAD(7328)", "HEAD(7360)", "HEAD(7392)", "HEAD(7424)", "HEAD(7456)", "HEAD(7488)", "HEAD(7520)", "HEAD(7552)", "HEAD(7584)", "HEAD(7616)", "HEAD(7648)", "HEAD(7680)", "HEAD(7712)", "HEAD(7744)", "HEAD(7776)", "HEAD(7808)", "HEAD(7840)", "HEAD(7872)", "HEAD(7904)", "HEAD(7936)", "HEAD(7968)", "HEAD(8000)", "HEAD(8032)", "HEAD(8064)", "HEAD(8096)", 
        "HEAD(8128)", "HEAD(MEDIUM)", "HEAD(WILD)", "INVALID"
    };
    
    inline AllocHeader* NextHeaderPtr(AllocHeader* h) {
        size_t sz = (size_t)h->thisSize.size;
        if (sz == 0) return h;
        return (internal::AllocHeader*)((char*)h + sz);
    }
    
    inline const char* StateText(AllocState s) {
        switch (s) {
            case AllocState::USED:                 return "USED";
            case AllocState::FREE:                 return "FREE";
            case AllocState::WILD_BLOCK:           return "WILD";
            case AllocState::BEGIN_SENTINEL:       return "SENTINEL B";
            case AllocState::LARGE_BLOCK_SENTINEL: return "SENTINEL L";
            case AllocState::END_SENTINEL:         return "SENTINEL E";
            default:                               return "INVALID";
        }
    }
    
    static inline constexpr const char* StateColor(AllocState s) {
        switch (s) {
            case AllocState::USED:               
                return DEBUG_ALLOC_COLOR_CYAN;
            case AllocState::FREE:   
            case AllocState::WILD_BLOCK: 
                return DEBUG_ALLOC_COLOR_GREEN;
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL: 
                return DEBUG_ALLOC_COLOR_YELLOW;
            case AllocState::INVALID: 
                return DEBUG_ALLOC_COLOR_RED;
            default: 
                return DEBUG_ALLOC_COLOR_RESET;
        }
    }
    
    static inline unsigned GetSmallBinIndexFromSize(uint64_t sz) {
        if (sz < 32) return 0u;
        if (sz <= 32ull * 253ull) return (unsigned)(sz / 32ull) - 1u;
        return 254u; // 254 = medium, 255 = wild
    }

    static inline void PrintRun(const char* s, int n, const char* color = DEBUG_ALLOC_COLOR_WHITE) {
        for (int i = 0; i < n; ++i) std::print("{}{}", color, s);
    }

    inline static unsigned GetFreeListBinIndex(const AllocHeader* h) {
        switch ((AllocState)h->thisSize.state) {
            case AllocState::WILD_BLOCK:
                return 255;
            case AllocState::FREE: 
            {
                Size sz = h->thisSize.size;
                if (sz >= 254ull * 32ull) return 254;
                else return (unsigned)(sz / 32ull);
            }
            case AllocState::INVALID:
            case AllocState::USED:
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL:
            default:
            {
                return 256;
            }
        }
    }

    // Build HEAD label based on bin index; bin 0..253 -> HEAD(size), 254 -> HEAD(MEDIUM), 255 -> HEAD(WILD)
    inline static const char* GetFreeListHeadLabel(AllocHeader* h) {
        unsigned binIdx = GetFreeListBinIndex(h);
        return DEBUG_FREELIST_HEAD_TABLE[binIdx];
    }

    // Fixed column widths (constants) in requested order
    constexpr int DEBUG_COL_W_OFF     = 18; // 0x + 16 hex
    constexpr int DEBUG_COL_W_SIZE    = 12;
    constexpr int DEBUG_COL_W_STATE   = 10;
    constexpr int DEBUG_COL_W_PSIZE   = 12;
    constexpr int DEBUG_COL_W_PSTATE  = 10;
    constexpr int DEBUG_COL_W_FL_PREV = 18;
    constexpr int DEBUG_COL_W_FL_NEXT = 18;

    static inline void PrintTopBorder() {
        std::print("{}┌{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┐{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintHeaderSeparator() {
        std::print("{}├{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┤{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintBottomBorder() {
        std::print("{}└{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┘{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintHeader() {
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "Offset");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "Size");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "State");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevSize");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevState");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListPrev");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListNext");
        std::print("{}│{}\n"     , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintRow(const AllocHeader* h) {
        const AllocTable* at = &gKernel.allocTable;
        uintptr_t beginAddr = (uintptr_t)at->beginSentinel;
        uintptr_t off = (uintptr_t)h - beginAddr;
        uint64_t  sz  = (uint64_t)h->thisSize.size;
        uint64_t  psz = (uint64_t)h->prevSize.size;
        AllocState st = (AllocState)h->thisSize.state;
        AllocState pst = (AllocState)h->prevSize.state;

        const char* stateText = StateText(st);
        const char* previousStateText = StateText(pst);
        const char* stateColor = StateColor(st);

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", stateColor, (unsigned long long)off);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)sz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, stateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)psz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, previousStateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", stateColor, "TODO");
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", stateColor, "TODO");
        std::print("{}│{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }
    
}

inline void DebugPrintAllocBlocks() noexcept 
{
    using namespace internal;
    assert(gKernel.allocTable.beginSentinel != nullptr);
    assert(gKernel.allocTable.endSentinel != nullptr);
    
    PrintTopBorder();
    PrintHeader();
    PrintHeaderSeparator();
    AllocHeader* head = (AllocHeader*) gKernel.allocTable.beginSentinel;
    AllocHeader* end  = (AllocHeader*) NextHeaderPtr(gKernel.allocTable.endSentinel);
    
    for (; head != end; head = NextHeaderPtr(head)) {
        PrintRow(head);
    }

    PrintBottomBorder();
}

} // namespace ak