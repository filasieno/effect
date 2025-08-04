#include "ak.hpp"
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

// Handle individual client connection
DefineTask HandleClient(int clientFd) noexcept {
    char buffer[1024];
    
    while (true) {
        // Read from client
        int bytes = co_await XRecv(clientFd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break; // Client disconnected or error
        }

        // Echo back
        co_await XSend(clientFd, buffer, bytes, 0);
    }

    // Close client connection
    co_await XClose(clientFd);
    co_return;
}

// Accept and handle new connections
DefineTask AcceptConnections(int serverFd) noexcept {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // Accept new connection
        int clientFd = co_await XAccept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen, 0);
        if (clientFd < 0) {
            continue;
        }

        // Handle client in a new task
        HandleClient(clientFd);
        // Note: We don't wait for the client handler to complete
    }
}

DefineTask MainTask() noexcept {
    // Create server socket
    int serverFd = co_await XSocket(AF_INET, SOCK_STREAM, 0, 0);
    if (serverFd < 0) {
        std::print("Failed to create socket\n");
        co_return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::print("Failed to set socket options\n");
        co_await XClose(serverFd);
        co_return;
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
        co_await XClose(serverFd);
        co_return;
    }

    // Listen
    if (listen(serverFd, SOMAXCONN) < 0) {
        std::print("Failed to listen\n");
        co_await XClose(serverFd);
        co_return;
    }

    std::print("Echo server listening on port 8080...\n");

    // Start accepting connections
    co_await AcceptConnections(serverFd);

    // Cleanup
    co_await XClose(serverFd);
    co_return;
}

int main() {
    KernelConfig config = {
        .mem = nullptr,
        .memSize = 0,
        .ioEntryCount = 256
    };
    return RunMain(&config, MainTask);
}