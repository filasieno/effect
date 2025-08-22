#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <format>

using namespace ak;

// Handle individual client connection
CThread processor_thread(int taskId,int clientFd) noexcept {
    char buffer[1024];
    
    while (true) {
        // Read from client
        int bytes = co_await io_recv(clientFd, buffer, sizeof(buffer), 0);
        std::print("Received from {}: {} bytes\n", taskId, bytes);
        if (bytes <= 0) {
            std::print("Received from {}: recv failed\n", taskId);
            co_await io_close(clientFd);
            break; // Client disconnected or error
        }

        // Echo back
        co_await io_send(clientFd, buffer, bytes, 0);
    }

    // Close client connection
    co_await io_close(clientFd);
    co_return 0;
}

// Accept and handle new connections
CThread acceptor_thread(int serverFd) noexcept {
    static int task_id = 0;
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // Accept new connection
        int clientFd = co_await io_accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen, 0);
        if (clientFd < 0) {
            continue;
        }
        std::print("Accepted client: {}\n", task_id);

        // Handle client in a new task
        processor_thread(task_id, clientFd);
        ++task_id;
        // Note: We don't wait for the client handler to complete
    }
}

CThread co_main() noexcept {
    int res;

    // Create server socket
    int serverFd = co_await io_socket(AF_INET, SOCK_STREAM, 0, 0);
    if (serverFd < 0) {
        std::print("Failed to create socket\n");
        co_return 0;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::print("Failed to set socket options\n");
        co_await io_close(serverFd);
        co_return 0;
    }

    // Setup server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // Bind
    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::print("Failed to bind\n");
        co_await io_close(serverFd);
        co_return 0;
    }

    // Listen
    if (listen(serverFd, SOMAXCONN) < 0) {
        std::print("Failed to listen\n");
        co_await io_close(serverFd);
        co_return 0;
    }

    std::print("Echo server listening on port 8080...\n");

    // Start accepting connections
    co_await acceptor_thread(serverFd);

    // Cleanup
    res = co_await io_close(serverFd);
    co_return 0;
}

int main() {
    KernelConfig config = {
        .mem = nullptr,
        .memSize = 0,
        .ioEntryCount = 256
    };
    init_kernel(&config);
    int res = run_main(co_main);
    fini_kernel();
    return res;
}