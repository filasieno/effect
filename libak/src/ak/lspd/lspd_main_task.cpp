#include "ak.hpp"   // IWYU pragma: keep
#include "lspd.hpp" // IWYU pragma: keep

alignas(4096) char buffer[4096];

AkTask lspd_main(LSPDConfig* config) noexcept 
{
    std::memset(buffer, 0, sizeof(buffer));
    std::print("lspd_main started\n");
    
    describe_transport(&config->transport, buffer, sizeof(buffer));
    std::print("transport: {}\n", buffer);

    if (config->transport.type == ldpd_transport_type::LSPD_TRANSPORT_TYPE_TCP_SOCKET) {
        std::print("TCP_SOCKET transport not supported yet\n");
        co_return -1;
    }

    if (config->transport.type == ldpd_transport_type::LSPD_TRANSPORT_TYPE_UNIX_SOCKET) {
        std::print("UNIX_SOCKET transport not supported yet\n");
        co_return -1;
    }

    co_return 0;
}
