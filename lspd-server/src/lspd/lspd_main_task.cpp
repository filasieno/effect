#include "ak.hpp"   // IWYU pragma: keep
#include "lspd.hpp" // IWYU pragma: keep

alignas(4096) char buffer[4096];

struct lspd_main_context g_lspd_main_context;

AkTask lspd_main() noexcept 
{
    std::memset(buffer, 0, sizeof(buffer));
    std::print("lspd_main started\n");
    
    auto* config = &g_lspd_main_context.config;

    describe_transport(&(config->transport), buffer, sizeof(buffer));
    std::print("transport: {}\n", buffer);

    if (config->transport.type == LSPD_TRANSPORT_TYPE_TCP_SOCKET) {
        std::print("TCP_SOCKET transport not supported yet\n");
        co_return -1;
    }

    if (config->transport.type == LSPD_TRANSPORT_TYPE_STDIO) {
        std::print("STDIO transport not supported yet\n");
        co_return -1;
    }

    // UNIX socket path selected => start acceptor
    if (config->transport.type == LSPD_TRANSPORT_TYPE_UNIX_SOCKET) {
        const char* unix_socket_path = config->transport.file_name;
        int acc_res = co_await lspd_acceptor_task(unix_socket_path);
        co_return acc_res;
    }

    co_return -1;
}
