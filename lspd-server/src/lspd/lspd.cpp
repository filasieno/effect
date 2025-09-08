#include <print>   // IWYU pragma: keep
#include "ak.hpp"  // IWYU pragma: keep
#include <argtable3.h>
#include "lspd.hpp"

AkTask lspd_main(LSPDConfig* config) noexcept;

int main(int argc, char** argv) {
    int res;
    LSPDConfig config;
    
    res = lspd_parse_args(argc, argv, &config);
    if (res != 0) {
        return res;
    }

    if (config.transport.type == LSPDTransportType::INVALID) {
        // INVALID Transport and no error -> help or version
        return 0;
    }

    constexpr size_t mem_buffer_size = 64 * 1024 * 1024;
    void* mem_buffer = malloc(mem_buffer_size);
    constexpr size_t io_ops = 1024;
    AkKernelConfig libak_cfg = {
        .mem_buffer           = mem_buffer,
        .mem_buffer_size      = mem_buffer_size,
        .io_uring_entry_count = io_ops
    };
    
    res = ak_init_kernel(&libak_cfg);
    if (res != 0) {
        return res;
    }

    res = ak_run_main(&lspd_main, &config);    

    ak_fini_kernel();
    
    return res;    
}