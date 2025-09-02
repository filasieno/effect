#pragma once

#include "ak/base/base.hpp" // IWYU pragma: keep
#include "ak.hpp"           // IWYU pragma: keep
#include <cstdio>

enum class LDSP_ERROR_CODE : int {
    SUCCESS = 0,
    INVALID_ARGUMENT = 300, //...
};

enum class LSPDTransportType {
    INVALID = 0,
    STDIO,
    UNIX_SOCKET,
    TCP_SOCKET
};

struct LSPDTransport {
    LSPDTransportType type;
    const char*       file_name;
    const char*       sock_addr;
    int               port;    
};

inline int describe_transport(const LSPDTransport* t, char* buffer, size_t buffer_size) noexcept {
    switch (t->type) {
    case LSPDTransportType::INVALID:
        return std::snprintf(buffer, buffer_size, "INVALID");
    case LSPDTransportType::STDIO:
        return std::snprintf(buffer, buffer_size, "STDIO");
    case LSPDTransportType::UNIX_SOCKET:
        return std::snprintf(buffer, buffer_size, "UNIX_SOCKET name: '%s'", t->file_name);
    case LSPDTransportType::TCP_SOCKET:
        return std::snprintf(buffer, buffer_size, "TCP_SOCKET addr: '%s' port: %d", t->sock_addr, t->port);
    default:
        std::abort();
    }
}

struct LSPDConfig {
    int           err; //< 0 on success, non-zero on error
    LSPDTransport transport;
};

/// \brief Parse the command line arguments and return the configuration
/// \param argc The number of command line arguments
/// \param argv The command line arguments
/// \param out_config The configuration to fill
/// \return 0 on success, non-zero on error
int lspd_parse_args(int argc, char** argv, LSPDConfig* out_config);



