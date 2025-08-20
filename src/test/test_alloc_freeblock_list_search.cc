#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print>

using namespace ak;
using namespace ak::priv;

static inline void reset_mask(U64* mask) { *mask = 0ull; }
static inline U32  bin_of(Size s)       { if (s==0) return 0; U64 b = (U64)((s-1) >> 5); return (U32)(b > 63 ? 63 : b); }

alignas(64) static U64 bit_mask;

CThread co_main() noexcept {
    U64* m = &bit_mask;

    // Mapping boundaries
    assert(bin_of(1)==0 && bin_of(32)==0 && bin_of(33)==1 && bin_of(2048)==63);

    // Empty mask -> not found (-1)
    reset_mask(m);
    assert(find_alloc_freelist_index(m,1)==-1);
    assert(find_alloc_freelist_index(m,2048)==-1);

    // Single bit exact matches
    reset_mask(m); set_alloc_freelist_mask(m,0);  assert(find_alloc_freelist_index(m,32)==0);
    reset_mask(m); set_alloc_freelist_mask(m,1);  assert(find_alloc_freelist_index(m,33)==1);
    reset_mask(m); set_alloc_freelist_mask(m,10); assert(find_alloc_freelist_index(m,321)==10);
    reset_mask(m); set_alloc_freelist_mask(m,62); assert(find_alloc_freelist_index(m,2016)==62);
    reset_mask(m); set_alloc_freelist_mask(m,63); assert(find_alloc_freelist_index(m,2000)==63);

    // Next set after required
    reset_mask(m); set_alloc_freelist_mask(m,5); set_alloc_freelist_mask(m,7);
    assert(find_alloc_freelist_index(m,(5*32)+1)==5);
    assert(find_alloc_freelist_index(m,(6*32)+1)==7);

    // Clear and get
    reset_mask(m); set_alloc_freelist_mask(m,0); set_alloc_freelist_mask(m,1);
    clear_alloc_freelist_mask(m,0);
    assert(!get_alloc_freelist_mask(m,0) && get_alloc_freelist_mask(m,1));
    assert(find_alloc_freelist_index(m,1)==1);

    // Cross extremes
    reset_mask(m); set_alloc_freelist_mask(m,63);
    assert(find_alloc_freelist_index(m,1u<<30)==-1); // > 2048 handled by large-tree path
    assert(find_alloc_freelist_index(m,0)==63);

    // Dense then gaps
    reset_mask(m);
    for (int i=0;i<=10;++i) set_alloc_freelist_mask(m,i);
    clear_alloc_freelist_mask(m,2);
    clear_alloc_freelist_mask(m,4);
    assert(find_alloc_freelist_index(m,1)==0);
    assert(find_alloc_freelist_index(m,65)==3);

    // All bits set -> any eligible required yields >= required; > 2048 is not eligible
    reset_mask(m); for (int i=0;i<64;++i) set_alloc_freelist_mask(m,i);
    assert(find_alloc_freelist_index(m,(64*32)+1) == -1);

    co_return 0;
}

static Char buffer[8192];
int main(){
    KernelConfig cfg{ .mem=buffer, .memSize=sizeof(buffer), .ioEntryCount=256 };
    if (run_main_cthread(&cfg, co_main) != 0) { std::print("main failed\n"); std::abort(); }
    return 0;
}