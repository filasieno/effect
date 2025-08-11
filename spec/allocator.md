# Custom Single-Threaded Memory Allocator Specification

## Overview

This specification describes a custom, single-threaded memory allocator optimized for medium-sized allocations (typically 64-4096 bytes) with rare small allocations (e.g., 32 bytes). The allocator is designed for efficiency in space and time, assuming a contiguous heap arena, 32-byte alignment for all blocks, and maximum block sizes ≤ 4GB (enforced via 32-bit size fields). It uses boundary-tag coalescing for O(1) merging of adjacent free blocks, segregated free lists with 64 size-class bins (each covering 32-byte ranges, e.g., bin 0: 32-63 bytes, bin 1: 64-95 bytes, up to bin 63: 1984-2015 bytes), and a 64-bit bitmap for fast identification of non-empty bins. For larger allocations (>2016 bytes), it falls back to a general best-fit search or direct mmap/sbrk if needed (though this spec focuses on the binned path).

Key features:

- **Header Overhead**: 16 bytes per block (compact, with bit-packing for flags).
- **Free List Management**: Doubly linked lists per bin, with pointers stored in the unused body of free blocks to avoid header bloat.
- **Coalescing**: Immediate O(1) merging using prevSize/thisSize fields, with re-binning post-merge.
- **Debug/Sanity**: Metadata field includes allocation type (16 bits) and a simple hash (48 bits) for corruption detection.
- **Alignment**: All blocks (header + payload) are multiples of 32 bytes; headers start on 32-byte boundaries.
- **Assumptions**:
  - Single-threaded: No locks or atomics.
  - Heap is contiguous (starting from a base address, grown via sbrk/mmap as needed).
  - Minimum payload: 16 bytes (total min block: 32 bytes = 16 header + 16 payload).
  - Block sizes ≤ 4GB (UINT32_MAX); enforced with checks.
  - Rare small allocations: Optimized for medium churn; tiny objects should use a separate pool if frequent.
  - No external fragmentation handling beyond coalescing (e.g., no compaction).
- **Performance Goals**: O(1) average-case allocation/free via binning and bitmap; low fragmentation through segregation and immediate coalescing.
- **Error Handling**: Returns nullptr on failure (e.g., out-of-memory, corruption); assumes debug builds validate hashes.
- **Portability**: Pseudocode is language-agnostic but assumes 64-bit architecture (for pointers and metadata).

The allocator is inspired by dlmalloc/jemalloc but simplified for single-threaded use, with custom metadata and bitmap acceleration.

## Global Data Structure: AllocTable

The `AllocTable` is a global singleton structure that manages the allocator's state. It includes the segregated bins, bitmap for quick bin status checks, heap boundaries, and optional statistics. It is initialized once at startup and accessed globally (no thread-safety needed).

### Definition

```text
structure AllocTable {
    // Segregated free lists: Array of 64 bins, each a doubly linked list head/tail.
    // Each bin covers a 32-byte size range: bin i for sizes [32 + i*32, 32 + (i+1)*32 - 1] bytes (total block size, including header).
    // Lists are doubly linked for O(1) insert/remove; pointers stored in free block bodies.
    array<Bin, 64> bins;  // Where Bin is { pointer<AllocHeader> head; pointer<AllocHeader> tail; }

    // Bitmap: 64 bits, one per bin. Bit i set if bins[i] has at least one free block.
    // Used for O(1) checks (e.g., non-empty bins via popcount) and fast find-next (via ctz on masked bitmap).
    uint64 bitmap;

    // Heap boundaries: Base address of the managed arena and current end (for growth).
    pointer<void> heap_base;
    pointer<void> heap_end;

    // Statistics (optional, for debugging/profiling):
    uint64 total_allocated_bytes;  // Sum of all allocated block sizes (including headers).
    uint64 total_free_bytes;       // Sum of free block sizes.
    uint64 peak_usage;             // Max total_allocated_bytes observed.
    uint32 num_allocations;        // Count of successful alloc calls.
    uint32 num_frees;              // Count of free calls.

    // Configuration constants (set at init):
    const uint32 ALIGNMENT = 32;             // Block alignment in bytes.
    const uint32 HEADER_SIZE = 16;           // Fixed header size.
    const uint32 MIN_BLOCK_SIZE = 32;        // Min total block (header + min payload 16B).
    const uint32 MAX_BLOCK_SIZE = 0xFFFFFFFF;  // 4GB limit (uint32 max).
    const uint32 NUM_BINS = 64;              // Fixed bins.
    const uint32 BIN_GRANULARITY = 32;       // Size increment per bin.
}
```

### Initialization

The `AllocTable` is initialized with a large initial arena (e.g., via mmap/sbrk). The entire arena starts as one large free block in the highest bin.

Pseudocode:

```text
function init_alloc_table(pointer<void> initial_arena, size_t initial_size) {
    alloc_table.heap_base = initial_arena;
    alloc_table.heap_end = initial_arena + initial_size;
    alloc_table.bitmap = 0;
    for i in 0 to NUM_BINS-1 {
        alloc_table.bins[i].head = null;
        alloc_table.bins[i].tail = null;
    }
    alloc_table.total_allocated_bytes = 0;
    alloc_table.total_free_bytes = initial_size;
    alloc_table.peak_usage = 0;
    alloc_table.num_allocations = 0;
    alloc_table.num_frees = 0;

    // Create initial free block covering the entire heap.
    pointer<AllocHeader> initial_block = (pointer<AllocHeader>) initial_arena;
    initial_block.prevSize = 0;  // No previous block.
    initial_block.thisSize = initial_size | (1 << 0);  // Set is_free flag (bit 0).
    initial_block.metadata = compute_metadata_hash(initial_block.prevSize, initial_block.thisSize, 0);  // Type=0 (general).

    // Insert into appropriate bin (likely the last one for large size).
    int bin_index = get_bin_index(initial_size);
    insert_to_bin(bin_index, initial_block);
    set_bitmap_bit(bin_index);
}
```

### Helper Functions for AllocTable

- `get_bin_index(size_t total_size)`: Computes bin as `((total_size - MIN_BLOCK_SIZE) / BIN_GRANULARITY)`. Clamps to 0..63; for > max bin, use special handling (e.g., direct allocation).
- `set_bitmap_bit(int bin)`: `alloc_table.bitmap |= (1ULL << bin);`
- `clear_bitmap_bit(int bin)`: `alloc_table.bitmap &= ~(1ULL << bin); if (bins[bin].head == null) clear.`
- `find_next_non_empty_bin(int start_bin)`: Mask bitmap >= start_bin, use ctz (count trailing zeros) on the masked value to find lowest set bit.
- `is_bitmap_empty()`: `popcount(alloc_table.bitmap) == 0` (using hardware popcnt if available).
- `grow_heap(size_t additional_size)`: Use sbrk/mmap to extend heap_end, add new free block, coalesce if adjacent.

## AllocHeader Description

Each memory block (allocated or free) begins with a fixed 16-byte `AllocHeader`. It uses boundary tags (prevSize/thisSize) for O(1) neighbor computation and coalescing. Flags are bit-packed into the low 5 bits of size fields (since sizes are multiples of 32, low 5 bits are free). The metadata field provides type tagging and sanity hashing.

### Structure

```text
structure AllocHeader {
    uint32 prevSize;    // Size of the previous physical block (actual = prevSize & ~0x1F; low 5 bits optional extra flags, e.g., bit 0: prev_is_free if needed for optimization).
    uint32 thisSize;    // Size of this block (actual = thisSize & ~0x1F; low 5 bits: bit 0 = is_free, bits 1-4 = custom flags, e.g., bit 1 = poisoned, bit 2 = sentinel).
    uint64 metadata;    // Low 16 bits: allocation type (user-defined, e.g., 0=general, 1=string, 2=array). High 48 bits: simple hash (e.g., XOR or CRC of prevSize ^ thisSize ^ type) for corruption detection.
}
```

### Details

- **Size Fields**: Actual size = `field & ~0x1F` (mask low 5 bits). Enforces multiples of 32. For free blocks, is_free = `thisSize & 1`.
- **Flags**: Low bits in thisSize: Essential is_free (bit 0). Extras: e.g., bit 1 for "prev_free" optimization (avoids deref), bit 2 for debug poison. prevSize low bits can mirror prev's thisSize flags.
- **Metadata**:
  - Type: 16 bits allow 65k categories; set on alloc, persists on free for stats.
  - Hash: 48 bits suffice for low collision; compute as `hash = (prevSize ^ thisSize ^ type) rolled or CRC32`. Validate on free/coalesce: If mismatch, assume corruption (abort or log).
- **Free Block Extension**: When free (is_free=1), the first 16 bytes of the body (after header) store the doubly linked list pointers:
  ```
  pointer<AllocHeader> prev_free;  // Previous in bin's list.
  pointer<AllocHeader> next_free;  // Next in bin's list.
  ```
  Access: `pointer<AllocHeader*> links = (pointer<AllocHeader*>)(header + HEADER_SIZE); links[0] = prev_free; links[1] = next_free;`
- **Allocated Block**: Body is user payload, starting at header + HEADER_SIZE, size = (thisSize & ~0x1F) - HEADER_SIZE.
- **Sentinel Blocks**: Optional head/tail sentinels at heap_base and heap_end (zero-size, not free) for boundary checks.
- **Validation**: On ops, check: Address % 32 == 0, size % 32 == 0, hash matches, no overflow (size <= MAX_BLOCK_SIZE).
- **Overhead Analysis**: 16B/header + 16B/links (free only) = ~12% for 128B block, ~6% for 256B. Min free block: 32B (header + links).

## Pseudocode

### Helper Functions

```text
function compute_metadata_hash(uint32 prev, uint32 this, uint16 type) -> uint64 {
    uint64 combined = ((uint64)prev << 32) | this;
    uint64 hash = simple_crc_or_xor(combined);  // E.g., hash = combined ^ (combined >> 16) ^ (combined >> 32);
    return (hash << 16) | type;  // High 48: hash, low 16: type.
}

function validate_header(pointer<AllocHeader> header) {
    uint16 type = header.metadata & 0xFFFF;
    uint64 expected_hash = compute_metadata_hash(header.prevSize, header.thisSize, type) >> 16;
    if ((header.metadata >> 16) != expected_hash) {
        error("Header corruption detected");
    }
    if (((pointer<void>)header % ALIGNMENT) != 0 || (header.thisSize & ~0x1F) % ALIGNMENT != 0) {
        error("Alignment violation");
    }
}

function get_neighbor_prev(pointer<AllocHeader> current) -> pointer<AllocHeader> {
    return current - (current.prevSize & ~0x1F);
}

function get_neighbor_next(pointer<AllocHeader> current) -> pointer<AllocHeader> {
    return current + (current.thisSize & ~0x1F);
}

function is_free(pointer<AllocHeader> header) -> bool {
    return (header.thisSize & 1) != 0;
}

function insert_to_bin(int bin, pointer<AllocHeader> block) {
    validate_header(block);
    pointer<AllocHeader*> links = (pointer<AllocHeader*>)(block + HEADER_SIZE);
    links[0] = alloc_table.bins[bin].tail;  // prev_free
    links[1] = null;  // next_free
    if (alloc_table.bins[bin].tail != null) {
        pointer<AllocHeader*> tail_links = (pointer<AllocHeader*>)(alloc_table.bins[bin].tail + HEADER_SIZE);
        tail_links[1] = block;
    } else {
        alloc_table.bins[bin].head = block;
    }
    alloc_table.bins[bin].tail = block;
    set_bitmap_bit(bin);
}

function remove_from_bin(pointer<AllocHeader> block) {
    pointer<AllocHeader*> links = (pointer<AllocHeader*>)(block + HEADER_SIZE);
    int bin = get_bin_index(block.thisSize & ~0x1F);
    if (links[0] != null) {
        pointer<AllocHeader*> prev_links = (pointer<AllocHeader*>)(links[0] + HEADER_SIZE);
        prev_links[1] = links[1];
    } else {
        alloc_table.bins[bin].head = links[1];
    }
    if (links[1] != null) {
        pointer<AllocHeader*> next_links = (pointer<AllocHeader*>)(links[1] + HEADER_SIZE);
        next_links[0] = links[0];
    } else {
        alloc_table.bins[bin].tail = links[0];
    }
    if (alloc_table.bins[bin].head == null) {
        clear_bitmap_bit(bin);
    }
}

function split_block(pointer<AllocHeader> block, size_t requested_total) -> pointer<AllocHeader> {
    size_t orig_size = block.thisSize & ~0x1F;
    if (orig_size < requested_total + MIN_BLOCK_SIZE) {
        return null;  // Can't split (need room for new header + min payload).
    }
    // Create new header for remainder.
    pointer<AllocHeader> remainder = block + requested_total;
    remainder.prevSize = requested_total;
    remainder.thisSize = (orig_size - requested_total) | 1;  // Free.
    remainder.metadata = compute_metadata_hash(remainder.prevSize, remainder.thisSize, 0);  // Type 0.

    // Update original block.
    block.thisSize = (requested_total & ~0x1F) | (block.thisSize & 0x1F);  // Keep flags, update size.
    block.metadata = compute_metadata_hash(block.prevSize, block.thisSize, (block.metadata & 0xFFFF));  // Update hash, keep type.

    // Update next neighbor's prevSize if exists.
    pointer<AllocHeader> next_after_remainder = get_neighbor_next(remainder);
    if (next_after_remainder != alloc_table.heap_end) {
        next_after_remainder.prevSize = remainder.thisSize;
    }

    // Insert remainder to its bin.
    int rem_bin = get_bin_index(remainder.thisSize & ~0x1F);
    insert_to_bin(rem_bin, remainder);

    return block;  // Return split block (now sized for request).
}

function coalesce_with_prev(pointer<AllocHeader> current) -> pointer<AllocHeader> {
    pointer<AllocHeader> prev = get_neighbor_prev(current);
    if (!is_free(prev)) return current;
    remove_from_bin(prev);  // Remove prev from its bin.

    size_t new_size = (prev.thisSize & ~0x1F) + (current.thisSize & ~0x1F);
    if (new_size > MAX_BLOCK_SIZE) error("Size overflow");

    prev.thisSize = new_size | 1;  // Free.
    prev.metadata = compute_metadata_hash(prev.prevSize, prev.thisSize, 0);  // Reset type to 0 on merge.

    // Update next's prevSize.
    pointer<AllocHeader> next = get_neighbor_next(prev);
    if (next != alloc_table.heap_end) {
        next.prevSize = prev.thisSize;
    }

    return prev;  // Merged block starts at prev.
}

function coalesce_with_next(pointer<AllocHeader> current) {
    pointer<AllocHeader> next = get_neighbor_next(current);
    if (next == alloc_table.heap_end || !is_free(next)) return;
    remove_from_bin(next);

    size_t new_size = (current.thisSize & ~0x1F) + (next.thisSize & ~0x1F);
    if (new_size > MAX_BLOCK_SIZE) error("Size overflow");

    current.thisSize = new_size | 1;
    current.metadata = compute_metadata_hash(current.prevSize, current.thisSize, 0);

    // Update next-next's prevSize.
    pointer<AllocHeader> next_next = get_neighbor_next(current);
    if (next_next != alloc_table.heap_end) {
        next_next.prevSize = current.thisSize;
    }
}
```

### Allocation Function

```text
function alloc(size_t requested_payload, uint16 alloc_type) -> pointer<void> {
    if (requested_payload == 0) return null;
    size_t total_requested = round_up(requested_payload + HEADER_SIZE, ALIGNMENT);
    if (total_requested < MIN_BLOCK_SIZE) total_requested = MIN_BLOCK_SIZE;
    if (total_requested > MAX_BLOCK_SIZE) return null;

    int target_bin = get_bin_index(total_requested);
    if (target_bin >= NUM_BINS) {
        // Large allocation: Search general free list or grow heap (simplified here).
        // For spec, assume fallback to highest bin or grow.
        target_bin = NUM_BINS - 1;
    }

    // Check exact bin first.
    pointer<AllocHeader> candidate = alloc_table.bins[target_bin].head;
    if (candidate != null) {
        // First-fit in bin: Take head if suitable.
        remove_from_bin(candidate);
    } else {
        // Find next larger non-empty bin via bitmap.
        int next_bin = find_next_non_empty_bin(target_bin + 1);
        if (next_bin == -1) {
            // No free blocks: Grow heap.
            size_t grow_amount = max(total_requested * 2, 4096);  // Heuristic.
            pointer<AllocHeader> new_block = grow_heap(grow_amount);
            if (new_block == null) return null;
            candidate = new_block;
        } else {
            candidate = alloc_table.bins[next_bin].head;  // Take head of larger bin.
            remove_from_bin(candidate);
        }
    }

    // Split if oversized.
    if ((candidate.thisSize & ~0x1F) > total_requested + MIN_BLOCK_SIZE) {
        candidate = split_block(candidate, total_requested);
    }

    // Mark allocated.
    candidate.thisSize &= ~1;  // Clear is_free.
    candidate.metadata = compute_metadata_hash(candidate.prevSize, candidate.thisSize, alloc_type);

    // Update stats.
    alloc_table.total_allocated_bytes += total_requested;
    alloc_table.total_free_bytes -= total_requested;
    alloc_table.peak_usage = max(alloc_table.peak_usage, alloc_table.total_allocated_bytes);
    alloc_table.num_allocations++;

    return (pointer<void>)(candidate + HEADER_SIZE);
}
```

### Free Function

```text
function free(pointer<void> ptr) {
    if (ptr == null) return;
    pointer<AllocHeader> header = (pointer<AllocHeader>)(ptr - HEADER_SIZE);
    validate_header(header);
    if (is_free(header)) error("Double free");

    size_t block_size = header.thisSize & ~0x1F;

    // Coalesce with neighbors.
    header = coalesce_with_prev(header);  // May shift header left.
    coalesce_with_next(header);  // Extends current.

    // Mark free and update hash (type reset to 0 on free? Or preserve for stats).
    header.thisSize |= 1;
    header.metadata = compute_metadata_hash(header.prevSize, header.thisSize, 0);

    // Insert to bin.
    int bin = get_bin_index(header.thisSize & ~0x1F);
    insert_to_bin(bin, header);

    // Update stats.
    alloc_table.total_allocated_bytes -= block_size;
    alloc_table.total_free_bytes += block_size;
    alloc_table.num_frees++;
}
```

### Additional Functions

- **realloc**: Free old, alloc new, copy data (handle in-place resize if space after).
- **calloc**: Alloc + zero payload.
- **Heap Validation**: Walk from heap_base via thisSize jumps, check hashes, alignments, free list integrity.
- **Stats Query**: Return AllocTable stats (e.g., fragmentation = 1 - (largest_free / total_free)).
- **Error Recovery**: On corruption, optionally mark block as poisoned (set flag) and skip.
