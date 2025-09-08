#include "lspd.hpp"

#include <argtable3.h>
#include <ak/runtime/runtime.hpp>

constexpr char prog_name[] = "lspd";
constexpr char help_header[] = R"(Select transport: stdio or socket)";

struct Args {
    struct arg_lit *sock;
    struct arg_lit *stdio;
    struct arg_lit *verbose;
    struct arg_lit *version;
    struct arg_lit *help;
    struct arg_int *port;
    struct arg_str *named;
    struct arg_str *addr;
    struct arg_end *end;
} args;

void *argtable[] = {
    args.stdio = arg_lit0(NULL, "stdio", "use stdio [default]"),
    args.sock = arg_lit0(NULL, "sock", "use socket transport"),
    args.named = arg_str0("n", "named", "PATH", "unix domain socket (with --sock)"),
    args.addr = arg_str0("a", "addr", "ADDR", "tcp socket address (with --sock) [defaults to 127.0.0.1]"),
    args.port = arg_int0(NULL, "port", "N", "tcp port (with --sock) [defaults to 2026]"),
    args.version = arg_lit0("v", "version", "show version and exit"),
    args.help = arg_lit0("h", "help", "show help and exit"),
    args.end = arg_end(32),
};

void print_help(void **argtable) noexcept 
{
    std::print("lspd v0.0.1; Usage: {}", prog_name);
    arg_print_syntax(stdout, argtable, "");
    std::print("\n{}\nOPTIONS:\n", help_header);
    arg_print_glossary(stdout, argtable, "  %-25s %s\n");
}

int parse_transport(LSPDConfig *out_config) 
{
    // Enforce mutually exclusive mode with default
    int mode_count = args.stdio->count + args.sock->count;
    if (mode_count == 0) {
        args.stdio->count = 1;
        mode_count = 1;
        out_config->transport.type = LSPDTransportType::STDIO;
        return 0;
    }
    if (mode_count > 1) {
        std::fprintf(stderr, "%s: choose exactly one of --stdio or --sock; the default is --stdio\n", prog_name);
        arg_print_errors(stderr, args.end, prog_name);
        return 1;
    }

    // Socket Case
    if (args.sock->count) {
        // Ensure that either file or an addr is provided
        if (args.named->count == 0 && args.addr->count == 0) {
            out_config->transport.type = LSPDTransportType::TCP_SOCKET;
            out_config->transport.sock_addr = "127.0.0.1";

            // if port is not set it to the default port 2026
            if (args.port->count == 0) {
                out_config->transport.port = 2026;
            } else {
                out_config->transport.port = args.port->ival[0];
            }
            return 0;
        }

        if (args.named->count >= 1 && args.addr->count >= 1) {
            std::fprintf(stderr, "%s: --sock requires either --file or --addr not both\n", prog_name);
            arg_print_errors(stderr, args.end, prog_name);
            return 1;
        }

        // Named Socket case
        if (args.named->count) {
            // enure that addr is not set with a named socket
            if (args.addr->count) {
                std::fprintf(stderr, "%s: --sock --named and --addr cannot be used together\n", prog_name);
                arg_print_errors(stderr, args.end, prog_name);
                return 1;
            }

            // endure that port is not used with a named socket
            if (args.port->count) {
                std::fprintf(stderr, "%s: --sock --named and --port cannot be used together\n", prog_name);
                arg_print_errors(stderr, args.end, prog_name);
                return 1;
            }
            // set the transport type and file name
            out_config->transport.type = LSPDTransportType::UNIX_SOCKET;
            out_config->transport.file_name = args.named->sval[0];
            return 0;
        }

        // Addr case
        if (args.addr->count) {
            out_config->transport.type = LSPDTransportType::TCP_SOCKET;

            // if addr is not set the it is set by default to 127.0.0.1
            if (args.addr->count == 0) {
                out_config->transport.sock_addr = "127.0.0.1";
            } else {
                out_config->transport.sock_addr = args.addr->sval[0];
            }

            // if port is not set it to the default port 2026
            if (args.port->count == 0) {
                out_config->transport.port = 2026;
            } else {
                out_config->transport.port = args.port->ival[0];
            }
            return 0;
        }
    }

    if (args.stdio->count) {
        out_config->transport.type = LSPDTransportType::STDIO;
        return 0;
    }

    std::abort();
}

int lspd_parse_args(int argc, char **argv, LSPDConfig *out_config) 
{

    int nerrors = arg_parse(argc, argv, argtable);

    // Count and version have priority over other methods
    if (args.help->count) {
        print_help(argtable);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        out_config->transport.type = LSPDTransportType::INVALID;
        return 0;
    }
    if (args.version->count) {
        std::print("lspd v0.0.1\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        out_config->transport.type = LSPDTransportType::INVALID;
        return 0;
    }

    // Check for parse errors
    if (nerrors > 0) {
        arg_print_errors(stderr, args.end, prog_name);
        std::fprintf(stderr, "Try '%s --help' for more information.\n", prog_name);
        return 1;
    }
    int res;

    res = parse_transport(out_config);
    if (res != 0) {
        return res;
    }

    return 0;
}
