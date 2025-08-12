#define AK_IMPLEMENTATION
#include "ak.hpp"
#include <cassert>
#include <print> 

using namespace ak;

static void ResetBitField(char* bitField) {
    memset(bitField, 0, 64);
    internal::SetFreeListBit(bitField, 255);
}
static char bitFieldStorage[256] = {0};
DefineTask MainTask() noexcept {
	char* bitField = (char*)(((uintptr_t)&bitFieldStorage) & ~63ull);
    
    // Test 1: allocSize=0, requiredBin=0, bit 0 set
    std::print("Test 1: allocSize=0, requiredBin=0, bit 0 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 0);
    assert(internal::FindFreeListBucket(0, bitField) == 0);

    // Test 2: allocSize=1, requiredBin=0, bit 0 not set, bit 1 set
    std::print("Test 2: allocSize=1, requiredBin=0, bit 0 not set, bit 1 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 1);
    assert(internal::FindFreeListBucket(1, bitField) == 1);

    // Test 3: allocSize=32, requiredBin=0, only wild set
    std::print("Test 3: allocSize=32, requiredBin=0, only wild set\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(32, bitField) == 255);

    // Test 4: allocSize=33, requiredBin=1, bit 1 set
    std::print("Test 4: allocSize=33, requiredBin=1, bit 1 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 1);
    assert(internal::FindFreeListBucket(33, bitField) == 1);

    // Test 5: allocSize=8161 (32*255 +1), requiredBin=255, wild set
    std::print("Test 5: allocSize=8161 (32*255 +1), requiredBin=255, wild set\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(8161, bitField) == 255);

    // Test 6: large allocSize=10000, clamped to 255
    std::print("Test 6: large allocSize=10000, clamped to 255\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(10000, bitField) == 255);

    // Test 7: requiredBin=254, bit 254 set
    std::print("Test 7: requiredBin=254, bit 254 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 254);
    unsigned req254 = (254 + 1) * 32 - 31;  // smallest size for bin 254: 32*254 +1 = 8129? Wait, adjust
    assert(internal::FindFreeListBucket(req254, bitField) == 254);

    // Test 8: requiredBin=254, bit 254 not set, returns 255
    std::print("Test 8: requiredBin=254, bit 254 not set, returns 255\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(req254, bitField) == 255);

    // Test 9: middle bin, e.g. 64, set
    std::print("Test 9: middle bin, e.g. 64, set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 64);
    unsigned req64 = 32 * 64 + 1;  // for bin64: ceil(s/32)-1=64 => ceil(s/32)=65 => s <=32*65, s>=32*64 +1=2049
    assert(internal::FindFreeListBucket(req64, bitField) == 64);

    // Test 10: middle bin 64 not set, but 65 set
    std::print("Test 10: middle bin 64 not set, but 65 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 65);
    assert(internal::FindFreeListBucket(req64, bitField) == 65);

    // Test 11: multiple bits set, picks smallest >= required
    std::print("Test 11: multiple bits set, picks smallest >= required\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 10);
    internal::SetFreeListBit(bitField, 12);
    internal::SetFreeListBit(bitField, 15);
    unsigned req11 = 32 * 11 + 1;  // requiredBin=11
    assert(internal::FindFreeListBucket(req11, bitField) == 12);  // smallest >=11 is 12 (since 10<11, skipped)

    // Test 12: demonstrate clear: set 0 and 1, clear 0, check get and find
    std::print("Test 12: demonstrate clear: set 0 and 1, clear 0, check get and find\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 0);
    internal::SetFreeListBit(bitField, 1);
    internal::ClearFreeListBit(bitField, 0);
    assert(!internal::GetFreeListBit(bitField, 0));
    assert(internal::GetFreeListBit(bitField, 1));
    assert(internal::FindFreeListBucket(1, bitField) == 1);

    // Test 13: allocSize=32 (exact for bin0), bit0 set
    std::print("Test 13: allocSize=32 (exact for bin0), bit0 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 0);
    assert(internal::FindFreeListBucket(32, bitField) == 0);

    // Test 14: allocSize=8160 (exact for bin254), bit254 set
    std::print("Test 14: allocSize=8160 (exact for bin254), bit254 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 254);
    assert(internal::FindFreeListBucket(8160, bitField) == 254);

    // Test 15: allocSize=8160, bit254 not set, falls to 255
    std::print("Test 15: allocSize=8160, bit254 not set, falls to 255\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(8160, bitField) == 255);

    // Test 16: requiredBin=255, allocSize=8161, returns 255
    std::print("Test 16: requiredBin=255, allocSize=8161, returns 255\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(8161, bitField) == 255);

    // Test 17: all bits set, picks exact requiredBin=100
    std::print("Test 17: all bits set, picks exact requiredBin=100\n");
    ResetBitField(bitField);
    for (int i = 0; i < 256; ++i) internal::SetFreeListBit(bitField, i);  // Set all
    unsigned req100 = 32 * 100 + 1;
    assert(internal::FindFreeListBucket(req100, bitField) == 100);

    // Test 18: cross word boundary, requiredBin=63, bit63 set
    std::print("Test 18: cross word boundary, requiredBin=63, bit63 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 63);
    unsigned req63 = 32 * 63 + 1;  // 2017 to 2048 for bin63
    assert(internal::FindFreeListBucket(req63, bitField) == 63);

    // Test 19: requiredBin=63, bit63 not set, but bit64 set
    std::print("Test 19: requiredBin=63, bit63 not set, but bit64 set\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 64);
    assert(internal::FindFreeListBucket(req63, bitField) == 64);

    // Test 20: clear multiple, set 0-10, clear 2,4,6,8, ensure picks 1 if >=req
    std::print("Test 20: clear multiple, set 0-10, clear 2,4,6,8, ensure picks 1 if >=req\n");
    ResetBitField(bitField);
    for (int i = 0; i <= 10; ++i) internal::SetFreeListBit(bitField, i);
    internal::ClearFreeListBit(bitField, 2);
    internal::ClearFreeListBit(bitField, 4);
    internal::ClearFreeListBit(bitField, 6);
    internal::ClearFreeListBit(bitField, 8);
    unsigned req1 = 1;  // requiredBin=0
    assert(internal::FindFreeListBucket(req1, bitField) == 0);  // 0 is set
    unsigned req3 = 33;  // requiredBin=1
    assert(internal::FindFreeListBucket(req3, bitField) == 1);  // 1 set, 2 cleared

    // Test 21: requiredBin=127, bit127 set (end of second word)
    std::print("Test 21: requiredBin=127, bit127 set (end of second word)\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 127);
    unsigned req127 = 32 * 127 + 1;
    assert(internal::FindFreeListBucket(req127, bitField) == 127);

    // Test 22: requiredBin=128, bit128 set (start of third word)
    std::print("Test 22: requiredBin=128, bit128 set (start of third word)\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 128);
    unsigned req128 = 32 * 128 + 1;
    assert(internal::FindFreeListBucket(req128, bitField) == 128);

    // Test 23: no bits set from 200 to 254, falls to 255
    std::print("Test 23: no bits set from 200 to 254, falls to 255\n");
    ResetBitField(bitField);
    for (int i = 0; i < 200; ++i) internal::SetFreeListBit(bitField, i);
    unsigned req200 = 32 * 200 + 1;
    assert(internal::FindFreeListBucket(req200, bitField) == 255);  // since 200-254 not set

    // Test 24: verify Get after Set and Clear across bytes
    std::print("Test 24: verify Get after Set and Clear across bytes\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 7);   // last bit in byte 0
    internal::SetFreeListBit(bitField, 8);   // first in byte 1
    assert(internal::GetFreeListBit(bitField, 7));
    assert(internal::GetFreeListBit(bitField, 8));
    internal::ClearFreeListBit(bitField, 7);
    assert(!internal::GetFreeListBit(bitField, 7));
    assert(internal::GetFreeListBit(bitField, 8));

    // Test 25: minimal allocSize=0, no other bits, falls to 255? But 0 should allow bin0 if set, but test unset
    std::print("Test 25: minimal allocSize=0, no other bits, falls to 255? But 0 should allow bin0 if set, but test unset\n");
    ResetBitField(bitField);
    internal::ClearFreeListBit(bitField, 255);  // Temporarily clear wild for test (though design says always set)
    assert(internal::FindFreeListBucket(0, bitField) == 255);  // Should return 255 even if cleared, per code
    internal::SetFreeListBit(bitField, 255);  // Restore

    // Test 26: maximal requiredBin=255, clear 255, but per code it checks mask, might return 255 anyway? Wait, if mask allows
    std::print("Test 26: maximal requiredBin=255, clear 255, but per code it checks mask, might return 255 anyway? Wait, if mask allows\n");
    ResetBitField(bitField);
    internal::ClearFreeListBit(bitField, 255);
    assert(internal::FindFreeListBucket(8161, bitField) == 255);  // Code has if non_zero_mask==0 return 255, so yes

    // Test 27: set every 8th bit, check skips correctly
    std::print("Test 27: set every 8th bit, check skips correctly\n");
    ResetBitField(bitField);
    for (int i = 0; i < 256; i += 8) internal::SetFreeListBit(bitField, i);
    unsigned req5 = 32 * 5 + 1;  // requiredBin=5
    assert(internal::FindFreeListBucket(req5, bitField) == 8);  // Next set is 8 >=5

    // Test 28: only bit 255 set, for various requiredBin
    std::print("Test 28: only bit 255 set, for various requiredBin\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(1, bitField) == 255);
    assert(internal::FindFreeListBucket(4000, bitField) == 255);
    assert(internal::FindFreeListBucket(10000, bitField) == 255);

    // Test 29: set bit 255 and bit 0, clear bit 0, check Get and find for small size
    std::print("Test 29: set bit 255 and bit 0, clear bit 0, check Get and find for small size\n");
    ResetBitField(bitField);
    internal::SetFreeListBit(bitField, 0);
    internal::ClearFreeListBit(bitField, 0);
    assert(!internal::GetFreeListBit(bitField, 0));
    assert(internal::FindFreeListBucket(1, bitField) == 255);

    // Test 30: very large allocSize, say 1<<30, clamps to 255
    std::print("Test 30: very large allocSize, say 1<<30, clamps to 255\n");
    ResetBitField(bitField);
    assert(internal::FindFreeListBucket(1ULL << 30, bitField) == 255);

    co_return;
}

char buffer[8192];

int main() {
	KernelConfig config = {
		.mem = buffer,
		.memSize = sizeof(buffer),
		.ioEntryCount = 256
  	};
	
	if (RunMain(&config, MainTask) != 0) {
		std::print("main failed\n");
		std::abort();
		// Unreachable
	}
	return 0;
}