#include <print>   // IWYU pragma: keep
#include "ak.hpp"  // IWYU pragma: keep
#include <argtable3.h>

static AkTask co_main() noexcept {
    std::print("lspd using libak v{}.{}.{} build {}\n", AK_MAYOR, AK_MINOR, AK_PATCH, AK_BUILD);
    co_return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    constexpr size_t mem_buffer_size = 64 * 1024 * 1024;
    void* mem_buffer = malloc(mem_buffer_size);
    constexpr size_t io_ops = 1024;
    AkKernelConfig libak_cfg = {
        .mem_buffer           = mem_buffer,
        .mem_buffer_size      = mem_buffer_size,
        .io_uring_entry_count = io_ops
    };
    
    int init_res = ak_init_kernel(&libak_cfg);
    if (init_res != 0) {
        return init_res;
    }

    int res = ak_run_main(co_main);
    ak_fini_kernel();
    return res;
}