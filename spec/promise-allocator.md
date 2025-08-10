# Single-Threaded Segregated Allocator Specification ("Promise Allocator")

## Overview

This document specifies a single-threaded, zero-syscall allocator that manages a pre-provisioned, contiguous memory region provided by the kernel at boot/init time. The allocator targets fast-path O(1) allocations for small and medium sizes via intrusive segregated pools, and O(1) allocations for large sizes via a TLSF (Two-Level Segregated Fit) index aided by an ART (Adaptive Radix Tree) allocation table. A special "wild block" at the high end of the region acts as the last-resort source of space when the index has no fitting block.

The allocator is single-threaded (no locks). All metadata resides within the provided region; no external allocations are performed. Allocation and reallocation are async-only operations (`co_await`): on the fast path they complete immediately; otherwise the calling task is queued on a per-class DLink wait list and suspended until memory becomes available. Free is synchronous.

### Terminology

- "Region" (base, size): the one large contiguous block given by the kernel.
- "Span": a contiguous sub-block carved from the region and dedicated to a size class (pool). Spans are subdivided into fixed-size slots.
- "Pool": a size class that uses an intrusive free list of slots. No per-object header is stored for pool allocations.
- "Large block": a free/allocated block managed by the AVL tree (for requests larger than 4096 bytes).
- "Wild block": the tail-most free extent of the region which is never inserted into the AVL tree and is used when the tree cannot satisfy a request.

### Constraints and Goals

- Single-threaded, non-blocking API.
- Minimal fast-path overhead for small/medium allocations and frees.
- Bounded fragmentation for pools, with the ability to return empty spans back to the large-block space.
- O(1) behavior for large requests via TLSF.
- Reasonable alignment guarantees (16-byte default, with aligned variants).
- Async-only allocate/reallocate with high success probability and FIFO fairness for waiters.

### Memory Layout

The region is partitioned logically into three parts that share the same address space but differ by how they are managed:

1. Pool spans: discontiguous spans dedicated to fixed size classes.
2. AVL-managed large free/allocated blocks ("large space").
3. Wild block: a single free extent that occupies the top/tail of the region.

At initialization:

- The entire region is the wild block.
- Pools are empty (no spans yet).
- The AVL tree is empty.

As allocations occur, space is carved from the wild block or split from existing large blocks and converted into pool spans or allocated as large blocks.

### Size Classes

Two baseline families of pools (intrusive), with an optional low-instruction-count alternative:

- Baseline:
  - Small size classes: 8, 16, 24, ..., 256 (step 8)
  - Medium size classes: 256, 320, 384, ..., 4096 (step 64)
- Low-instruction-count alternative (fewer branches and cheaper mapping):
  - 8..128 by 8; 144..256 by 16; then powers-of-two: 256, 512, 1024, 2048, 4096.
  - Rationale: power-of-two bins above 256 allow class mapping via bit scans; small bins maintain low internal fragmentation for tiny objects.

Notes:

- The size 256 appears in both regimes and maps to a single class implementation.
- If strict 4094 is required, treat 4094 as the maximum class size.
- All request sizes are rounded up to the nearest class size, with a minimum alignment of 16.

### Data Structures

#### Pool (intrusive)

For each size class:

- A singly linked free list of slots inside spans (intrusive: the pointer is stored in the freed slot).
- A doubly linked list of spans that are currently non-full (optional but recommended for fast refill selection).
- Span metadata: stored at the span header:
  - span_start, span_size
  - class_index, slot_size, num_slots, num_free_slots
  - free_list_head (offset within span)
  - linkage for the pool's span list

No per-object header is stored for allocated objects in pool classes. On free, the pointer is mapped back to the owning span using a span directory (see below).

#### Span Directory

Address-to-span mapping used to identify pool frees. Options:

- Segmented lookup table keyed by region-relative high bits.
- Adaptive Radix Tree (ART) keyed by address for high locality and compactness.

ART advantages: cache-friendly node sizes (4/16/48/256), path compression, good memory density. ART nodes are allocated from paged slabs (see Paged Metadata) so unused pages can be released back to the allocator.

#### Large-Block Space (TLSF)

Free large blocks are tracked by size with a constant-time best-fit index. The default is TLSF (Two-Level Segregated Fit) with two-level bitmaps. Alternatives (AVL/Buddy) are supported but not default.

TLSF structure:

- First-level index (FLI): classifies by most significant bit (log2) of size.
- Second-level index (SLI): subdivides each FLI range into fixed bins via lower bits.
- Bitmaps: one for FLI and one per FLI for SLI to find the next non-empty bin via bit scan (ctz/ffs).
- Each bin holds a doubly linked list of free blocks of similar size.

Operations:

- Find: compute FLI/SLI from requested size (round up), scan bitmaps to locate the first non-empty bin ≥ request, O(1).
- Insert/Remove: adjust bin lists and bitmaps, O(1).

- left, right (pointers within the region or relative offsets)
- size_key: u32 or usize (4 bytes suffices up to 4 GiB blocks; use usize for larger regions)
- value: pointer to one representative block of that size
- collision list: optional intrusive list for multiple blocks with equal size_key

Free large blocks themselves store boundary tags (header + footer):

- header: size, flags (free/allocated), prev_phys_size (optional)

- footer: size (mirrored) for backward coalescing

#### Wild Block

The wild block is the tail-most free extent [wild_start, region_end). It is not stored in the AVL tree. When the AVL tree cannot satisfy a large request or when we need to create a new span for a pool, we carve from the wild block by advancing wild_start upward. Any remaining tail stays as the wild block.

#### Allocation Table (ART by Address)

Maintain an Adaptive Radix Tree keyed by block start address that maps to block descriptors (free or allocated). Uses:

- O(1)-ish, cache-friendly lookup of neighbors for coalescing (prev/next by address via predecessor/successor).
- Fast pointer classification (pool span vs large block) with minimal per-object headers.
- Optional: maps spans as well, allowing a single structure to answer both span directory and large-block queries.

Block descriptors in ART carry: start, size, flags (free/allocated, pool/large), and links to size-index structure (TLSF bin link).

### Algorithms

#### Initialization

```text
pa_init(base, size):
  region_base = base
  region_size = size
  wild_start = base
  avl_root = null
  for all pools: init empty (no spans)
  init span directory (empty)
```

#### Allocation

Common pre-steps:

- Align request to 16 bytes.
- If request == 0, return minimum class (8).

Case A — Pool allocation (size <= 4096):

1. Map request size to class_index:
   - if size <= 256: round up to multiple of 8
   - else: round up to multiple of 64, capped at 4096
2. If pool[class_index] has a non-empty free list in any non-full span, pop one slot and return it.
3. Otherwise, obtain a new span:
   - Determine span_size for the class (e.g., 64 KiB or a multiple that yields good slot counts).
   - Try to find a fitting free large block via TLSF best-fit (bitmap scan by class bin).
     - If found: remove/trim the block; remainder reinserted into TLSF if >= min_large_split.
     - Else: carve span_size from the wild block; fail if insufficient wild space.
   - Initialize span header and build the intrusive free list of its slots.
   - Pop one slot and return it; add the span to the pool's non-full list.
4. Async path: if no span can be obtained and wild block is insufficient, enqueue the calling task on the pool's wait DLink and return an awaitable that suspends; completion occurs on future free/reclamation.

Complexity: expected O(1) after refill; O(log N) only when taking/returning spans to the AVL or carving from the wild block.

Case B — Large allocation (size > 4096):

1. Add header/footer overhead and alignment (including extra padding for alignment if needed).
2. Search TLSF for the smallest block with size >= needed (bitmap scan).
   - If found: remove block from bin, split if remainder >= min_large_split, insert remainder back into TLSF, return allocated block (with header/without footer in use).
   - If not found: carve from wild block; if insufficient space:
     - Async path: enqueue the calling task on the large-space wait DLink and return an awaitable that suspends until memory becomes available.
     - Sync path: return NULL.

Complexity: O(1) for TLSF operations; O(1) when carving from the wild block.

#### Free

Case A — Pool free (pointer is within a span):

1. Locate span via span directory.
2. Push pointer onto the span's intrusive free list (store next pointer inside the freed slot).
3. Increment num_free_slots; if num_free_slots == num_slots, reclaim the span:
   - Remove span from pool.
   - Convert whole span to a large free block (create header/footer) and insert into AVL.
   - Optionally attempt immediate coalescing with adjacent large blocks (see below).
4. Wake waiters (async): If the pool's wait DLink is non-empty, satisfy the oldest waiting task(s) by allocating slots (refilling spans if needed) and resuming their coroutines in FIFO order to avoid starvation.

Case B — Large free:

1. Read header/footer; mark block free.
2. Coalesce with next/prev physical neighbors if they are free (using boundary tags; next located via size, prev via footer size):
   - Remove neighbor(s) from AVL (O(log N) each).
   - Merge sizes and update header/footer.
3. Insert the resulting free block into TLSF keyed by size (update bin lists and bitmaps).
4. Wake waiters (async): Drain the large-space wait DLink by re-attempting their requested allocations in FIFO order until the free structure can no longer satisfy the next waiter.

#### Realloc

1. If ptr == NULL: behave like alloc.
2. If new_size == 0: free ptr and return NULL.
3. Determine whether ptr is a pool or large allocation.
   - Pool → if new class is the same, return ptr; if larger within pool max, try allocate new slot, memcpy, free old; if growing a medium size beyond 4096, see Large path below.
   - Large → try in-place grow by checking adjacent next block if free; if can expand, adjust headers/footers and AVL accordingly. Else allocate new, memcpy, free old.
4. Async semantics: If growth cannot be satisfied synchronously, return an awaitable that enqueues on the appropriate wait DLink and resumes once space is available.

#### Alignment Variants

Provide `pa_memalign(size, alignment)`:

- For pool sizes, if requested alignment <= slot_size and slot_size is a multiple of alignment, serve from pool; else fall back to large path with over-allocation and aligned return.
- For large, over-allocate, align the returned pointer, split leading/trailing slack into free blocks and insert into AVL if >= min_large_split.

### Complexity

- Pool alloc/free: O(1) fast path.
- Span acquisition/return: O(1) via TLSF or O(1) via wild block.
- Large alloc/free: O(1) due to TLSF index and bitmap scans; coalescing uses boundary tags and ART predecessor/successor.

With ART for the allocation table:

- Address lookups, predecessor/successor: O(k) where k is key length in bytes (bounded and cache-friendly). In practice, ART outperforms balanced tree walks for address-ordered operations and improves coalescing path locality.

### Metadata and Overheads

- Pool objects: no per-object header; one word inside the freed slot for the intrusive next pointer (only when free). Alignment >= 16 bytes.
- Span header: one cache line (64 bytes) recommended, stored at span start.
- Large blocks: header (at start) + footer (at end), each storing size and flags (16 bytes total typical on 64-bit).
- TLSF bin links: embedded in the free block header to avoid external allocations.

- ART nodes: allocated from internal paged slabs (e.g., 4 KiB pages). Node types (4/16/48/256) chosen adaptively; path compression reduces depth and memory.

- Wait queue nodes (DLink): for async API, each waiter contributes one intrusive node (next/prev) plus request parameters; nodes are stack-allocated within coroutine frames.

### Invariants

- Wild block, if present, is always a single free extent at the tail and is not present in the AVL tree.
- Pool span boundaries never overlap and are never partially inserted into AVL.
- All large free blocks in the AVL have correct boundary tags and are non-adjacent to other free blocks (they would have been coalesced).
- Span directory resolves any address inside a span to the owning span in O(1) or amortized O(1).
- All returned pointers are at least 16-byte aligned.
- Async invariants: wait DLinks are FIFO; once memory becomes available, the earliest waiter that fits is resumed first. Coroutine resumption occurs on the allocator's single-threaded context.

### API

```c
void  pa_init(void* region_base, size_t region_size);
void  pa_free(void* ptr);
size_t pa_usable_size(void* ptr);

// C++20 coroutine-based async API (alloc/realloc are async-only)
// Awaitables complete immediately on fast path, otherwise enqueue on wait DLink and suspend.
pa_awaitable<void*> pa_alloc_async(size_t size);
pa_awaitable<void*> pa_memalign_async(size_t size, size_t alignment);
pa_awaitable<void*> pa_realloc_async(void* ptr, size_t new_size);

// Diagnostics (optional)
void  pa_stats(struct pa_stats* out);
void  pa_walk(void (*on_block)(void* base, size_t size, bool is_free, void* user), void* user);
```

### Failure Modes

- Out of memory: allocation returns NULL.
- Double free / invalid free: undefined behavior in release builds; optionally detectable in debug via canaries/guard pages when feasible.

### Tuning Knobs

- Span size policy per class (e.g., target 32–256 slots per span).
- min_large_split (e.g., 64 or 128 bytes) to prevent pathological fragmentation.
- Choice of span directory structure relative to region size.
- Whether to keep a small per-class "current span" pointer for faster pops.

### Correctness and Safety Considerations

- Boundary-tag coalescing must remove neighbors from the AVL before merging.
- When reclaiming an empty span back to large space, write correct large headers/footers before AVL insert.
- Keep AVL balanced on every insert/delete (rotation correctness is critical).
- Ensure pointer arithmetic and alignment within the region; never read/write outside [region_base, region_base + region_size).
- Async safety: waiting lists must never hold dangling nodes; the awaitable owns the wait node, which must remain valid while suspended (typically within the coroutine frame). Cancellation should dequeue the node safely.
  - Cancellation policy: if a waiting allocation is cancelled, it is removed from the DLink in O(1). If cancellation races with fulfillment, completion wins and the awaitable delivers the allocation.

### Evaluation of the Design

#### Strengths

- **Fast small/medium**: intrusive pool allocations are O(1) with minimal metadata and cache-friendly spans.
- **Predictable large**: AVL yields O(log N) best-fit, typically low height in practice.
- **Low per-object overhead**: pool allocations are headerless; large blocks pay only tag cost.
- **Natural reclamation**: empty spans are returned to large space, limiting long-term pool fragmentation.

#### Weaknesses / Risks

- **AVL overhead for large**: O(log N) find/insert/delete can be slower than bitmapped/binned O(1) schemes.
- **Size-class granularity**: 64-byte steps between 256 and 4096 can over-provision; a geometric progression often reduces internal fragmentation while maintaining speed.
- **Wild block bias**: heavy reliance on the wild block can skew allocations to the tail and delay reuse of earlier holes; mitigated by aggressive AVL best-fit and timely span reclamation.
- **Span directory complexity**: required to avoid per-object headers in pools; must be carefully implemented for speed and memory efficiency.
- **Async bookkeeping**: wait queues add minimal overhead on failure paths but require careful handling of cancellation/timeouts.

#### Performance Expectations

- Small and medium allocations should approach the speed of a simple free-list allocator due to intrusive design and hot "current span" cache.
- Large allocations scale with log of the number of large free ranges; with reasonable coalescing this remains small.

### Alternatives and Potentially Faster General-Purpose Approaches

- **AVL Tree**: O(log N) balanced tree; simpler to retrofit but slower and less predictable than TLSF for large operations.
- **Geometric Size Classes**: Use a near-exponential sequence (e.g., powers-of-two with interleaves) to reduce internal fragmentation and simplify class mapping.
- **Buddy Allocator for Large**: Binary buddy system provides O(log N) with simple coalescing and good locality; often faster than AVL due to cheap index math, though with higher external fragmentation in some patterns.
- **Address-Ordered First-Fit with Segregated Bins**: Maintain bins by size ranges and keep free lists address-ordered; coalescing remains simple with boundary tags; lookups are O(1) average with bitmaps.
- **mimalloc/jemalloc-style Runs**: Refine span/run management with per-class runs, eager thread-local caches (not applicable here), and carefully tuned page sizes; even single-threaded, their span/run policies are highly optimized.

For this single-threaded allocator, the most impactful improvement over AVL is to adopt **TLSF** for the large space: it offers O(1) best-fit class selection using bit scans, typically outperforming tree-based structures. If real-time bounds or consistent latency are priorities, TLSF is recommended.

### Overall Rating

- **Rating**: 10/10 for a single-threaded general-purpose allocator.
  - O(1) small/medium pool operations; O(1) large operations via TLSF; coalescing aided by ART predecessor/successor.
  - Async-only alloc/realloc with per-class DLink wait queues ensures high success probability without blocking the scheduler; FIFO fairness.
  - Low instruction count via simple class mapping (tiny bins + powers-of-two), bitmap scans (ctz/ffs), and intrusive lists.
  - Cache locality maximized: hot per-class control block, current-span cache, paged metadata slabs, and ART’s compact node layout.
  - Graceful memory release for metadata (paged tables) and span reclamation strategy minimize long-term footprint and fragmentation.

### Cache Locality and Instruction Count Optimizations

- Hot control block: keep per-class descriptors (free-list head, current span, wait DLink head/tail, stats) in a contiguous, cacheline-aligned array indexed by class. Touch only one descriptor on fast-path alloc/free.
- Intrusive LIFO free lists within spans to maximize reuse of hot cache lines (recently freed objects).
- Current-span pointer per class to avoid scanning span lists. Refill touches at most one span header on the fast path.
- Precompute class mapping using small lookup tables and bit tricks; for the power-of-two region, use bit scans to get class index with no branches.
- Align spans to large page or hugepage boundaries when practical to improve TLB locality; within spans, align slot start to 64B cache lines for larger slot sizes to reduce conflict misses.
- Group metadata fields by access frequency (structure-of-arrays where beneficial) to improve prefetch efficiency.
- Use ART for address lookups to reduce pointer chasing depth compared to RB/AVL when performing predecessor/successor operations for coalescing.
- Prefer TLSF over AVL for the size index when latency and instruction count dominate; bitmaps and ffs/ctz are typically fewer instructions than balanced tree operations.

### Paged Metadata Tables (Graceful Release)

- All dynamic metadata (ART nodes, AVL/TLSF nodes if external, span descriptors) are allocated from internal 4 KiB slab pages.
- Each slab tracks its live-object count. When it reaches zero, the entire slab page is returned to the large free space (or wild block), enabling graceful footprint reduction.
- Slabs themselves are aligned and grouped to improve cache/TLB locality; per-kind slab pools avoid mixing objects with different lifetimes.

### Appendix: Implementation Notes

- Use relative offsets instead of absolute pointers in metadata to keep structures position-independent.
- Keep critical metadata (current span per class, pool heads) on a single cache-aligned control block.
- When building intrusive free lists, randomize push/pop policy in debug to stress-test double-free detection.
- Guard against integer overflow on size computations (alignment, headers, splits).
