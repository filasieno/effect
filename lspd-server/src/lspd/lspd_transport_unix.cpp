#include "lspd.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

static inline socklen_t make_sockaddr_un(const char* path, struct sockaddr_un* out) noexcept {
    std::memset(out, 0, sizeof(*out));
    out->sun_family = AF_UNIX;
    std::strncpy(out->sun_path, path, sizeof(out->sun_path) - 1);
    return (socklen_t)(offsetof(struct sockaddr_un, sun_path) + std::strlen(out->sun_path) + 1);
}

AkTask lspd_jsonrpc_sender_task(int fd) noexcept
{
    static const char banner[] = "lspd ready\n";
    int wr = co_await ak_os_io_send(fd, banner, (size_t)sizeof(banner) - 1, 0);
    co_return (wr < 0) ? wr : 0;
}

AkTask lspd_jsonrpc_receiver_task(int fd) noexcept
{
    alignas(64) char buf[4096];
    while (true) {
        int rd = co_await ak_os_io_recv(fd, buf, sizeof(buf), 0);
        if (rd == 0) {
            // Peer closed
            break;
        }
        if (rd < 0) {
            co_return rd;
        }
        // TODO: parse JSON-RPC here. For now, just print byte count.
        std::print("recv {} bytes\n", rd);
    }
    co_return 0;
}

AkTask lspd_acceptor_task(const char* unix_socket_path) noexcept
{
    // Best-effort unlink existing path
    (void)co_await ak_os_io_unlink(unix_socket_path, 0);

    // Create socket
    int listen_fd = co_await ak_os_io_socket(AF_UNIX, SOCK_STREAM, 0, 0);
    if (listen_fd < 0) {
        std::print("socket() failed: {}\n", listen_fd);
        co_return listen_fd;
    }

    // Bind
    struct sockaddr_un addr;
    socklen_t addrlen = make_sockaddr_un(unix_socket_path, &addr);
#if defined(IORING_OP_BIND)
    {
        int br = co_await ak_os_io_bind(listen_fd, (struct sockaddr*)&addr, addrlen);
        if (br < 0) {
            std::print("bind() failed: {}\n", br);
            (void)co_await ak_os_io_close(listen_fd);
            co_return br;
        } 
    }
#else
    {
        int br = ::bind(listen_fd, (struct sockaddr*)&addr, addrlen);
        if (br != 0) {
            int err = -errno;
            std::print("bind() failed: {}\n", err);
            (void)co_await ak_os_io_close(listen_fd);
            co_return err;
        }
    }
#endif
    std::print("did bind unix domain docket at: '{}'", unix_socket_path);
    // Listen
#if defined(IORING_OP_LISTEN)
    {
        int lr = co_await ak_os_io_listen(listen_fd, 64);
        if (lr < 0) {
            std::print("listen() failed: {}\n", lr);
            (void)co_await ak_os_io_close(listen_fd);
            co_return lr;
        }
    }
#else
    if (::listen(listen_fd, 64) != 0) {
        int err = -errno;
        std::print("listen() failed: {}\n", err);
        (void)co_await ak_os_io_close(listen_fd);
        co_return err;
    }
#endif

    std::print("listening on unix domain socket at:{}\n", unix_socket_path);

    // Accept loop (sequential per connection for now)
    while (true) {
        int cfd = co_await ak_os_io_accept(listen_fd, nullptr, nullptr, 0);
        if (cfd < 0) {
            std::print("accept() failed: {}\n", cfd);
            break;
        }

        // Start per-connection tasks
        AkTask recv_task = lspd_jsonrpc_receiver_task(cfd);
        AkTask send_task = lspd_jsonrpc_sender_task(cfd);

        int recvr = co_await recv_task;
        (void)co_await ak_os_io_shutdown(cfd, SHUT_RDWR);
        (void)co_await send_task;
        (void)co_await ak_os_io_close(cfd);
        std::print("connection closed (receiver: {})\n", recvr);
    }

    // Iterate the sender and receiver Tasks
    // Close and Unlink the Acceptor Socket
    (void)co_await ak_os_io_close(listen_fd);
    (void)co_await ak_os_io_unlink(unix_socket_path, 0);
    co_return 0;
}


