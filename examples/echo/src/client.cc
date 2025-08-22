
#include "ak.hpp" // IWYU pragma: keep
#include <arpa/inet.h>
#include <cstring>
#include <print>
#include <vector>

using namespace ak;

CThread client_task(int taskId,const char* serverIp, int port, int msgPerClient) noexcept {
    // Create socket
    int sock = co_await io_socket(AF_INET, SOCK_STREAM, 0, 0);
    if (sock < 0) {
        std::print("Failed to create socket\n");
        co_return 0;
    }

    // Setup server address
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &server_addr.sin_addr) <= 0) {
        std::print("Invalid address\n");
        co_await io_close(sock);
        co_return 0;
    }

    // Connect to server
    int result = co_await io_connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
        std::print("Connection failed\n");
        co_await io_close(sock);
        co_return 0;
    }
    std::print("task {} connected to server\n", taskId);

    char buff[128];
    // TaskHdl current = co_await gKernel.currentTaskHdl;
    
    // Send multiple messages
    for (int i = 0; i < msgPerClient; i++) {
        // Prepare message
        int len = std::snprintf(buff, sizeof(buff), "Message %d from Task %d", i, taskId); 
        std::print("Client {} Received: {}\n", taskId, len); 
        // Send message
        result = co_await io_write(sock, buff, len, 0);
        if (result < 0) {
            std::print("Send failed\n");
            break;
        }


        // Receive echo
        result = co_await io_read(sock, buff, sizeof(buff)-1, 0);
        if (result < 0) {
            std::print("Receive failed\n");
            break;
        }
        buff[result] = '\0';
        
        std::print("Received: {}\n", buff);
    }

    co_await io_close(sock);
    co_return 0;
}

CThread co_main(int clientCount, int msgPerClient, const char* serverIp, int serverPort) noexcept {
    // Create array to store client task handles
    std::vector<CThread> clients(clientCount);

    // Launch client tasks
    for (int i = 0; i < clientCount; i++) {
        clients[i] = client_task(i, serverIp, serverPort, msgPerClient);
    }

    // Wait for all clients to complete
    for (auto& client : clients) {
        co_await client;
    }
    std::print("All clients completed\n");
    co_return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::print("Usage: {} <server-ip> <server-port> <client-count> <messages-per-client>\n", argv[0]); 
        return 1;
    }

    const char* serverIp = argv[1];
    int serverPort = std::atoi(argv[2]);
    int clientCount = std::atoi(argv[3]);
    int msgPerClient = std::atoi(argv[4]);

    // Configure kernel
    KernelConfig config{
        .mem = nullptr,
        .memSize = 0,
        .ioEntryCount = 1024
    };

    // Run the main task
    init_kernel(&config);
    int res = run_main(co_main, clientCount, msgPerClient, serverIp, serverPort);
    AK_ASSERT(res == 0);
    fini_kernel();
    return 0;
}