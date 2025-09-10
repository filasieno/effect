#pragma once

#include "ak/base/base.hpp" // IWYU pragma: keep
#include "ak.hpp"           // IWYU pragma: keep
#include <cstdio>

enum lspd_error_code {
    LSPD_ERROR_CODE_SUCCESS          = 0,
    LSPD_ERROR_CODE_INVALID_ARGUMENT = 300, //...
};

enum ldpd_transport_type {
    LSPD_TRANSPORT_TYPE_INVALID = 0,
    LSPD_TRANSPORT_TYPE_STDIO,
    LSPD_TRANSPORT_TYPE_UNIX_SOCKET,
    LSPD_TRANSPORT_TYPE_TCP_SOCKET
};

struct lspd_transport {
    ldpd_transport_type type;
    const char*         file_name;
    const char*         sock_addr;
    int                 port;    
};

inline int describe_transport(const lspd_transport* t, char* buffer, size_t buffer_size) noexcept {
    switch (t->type) {
    case LSPD_TRANSPORT_TYPE_INVALID:
        return std::snprintf(buffer, buffer_size, "INVALID");
    case LSPD_TRANSPORT_TYPE_STDIO:
        return std::snprintf(buffer, buffer_size, "STDIO");
    case LSPD_TRANSPORT_TYPE_UNIX_SOCKET:
        return std::snprintf(buffer, buffer_size, "UNIX_SOCKET name: '%s'", t->file_name);
    case LSPD_TRANSPORT_TYPE_TCP_SOCKET:
        return std::snprintf(buffer, buffer_size, "TCP_SOCKET addr: '%s' port: %d", t->sock_addr, t->port);
    default:
        std::abort();
    }
}

struct lspd_config {
    int            err; //< 0 on success, non-zero on error
    lspd_transport transport;
};


struct lspd_main_context {
    struct lspd_config config;
};
extern struct lspd_main_context g_lspd_main_context;

/// \brief Parse the command line arguments and return the configuration
/// \param argc The number of command line arguments
/// \param argv The command line arguments
/// \param out_config The configuration to fill
/// \return 0 on success, non-zero on error
int lspd_parse_args(int argc, char** argv, lspd_config* out_config);

// Tasks
AkTask lspd_main() noexcept;
AkTask lspd_acceptor_task(const char* unix_socket_path) noexcept;
AkTask lspd_jsonrpc_sender_task(int fd) noexcept;
AkTask lspd_jsonrpc_receiver_task(int fd) noexcept;

