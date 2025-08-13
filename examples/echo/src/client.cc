#include "ak.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <print>
#include <vector>

using namespace ak;


DefineTask ClientTask(int taskId,const char* serverIp, int port, int msgPerClient) noexcept {
    // Create socket
    int sock = co_await IOSocket(AF_INET, SOCK_STREAM, 0, 0);
    if (sock < 0) {
        std::print("Failed to create socket\n");
        co_return;
    }

    // Setup server address
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &server_addr.sin_addr) <= 0) {
        std::print("Invalid address\n");
        co_await IOClose(sock);
        co_return;
    }

    // Connect to server
    int result = co_await IOConnect(sock, (sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
        std::print("Connection failed\n");
        co_await IOClose(sock);
        co_return;
    }
    std::print("task {} connected to server\n", taskId);

    char buff[128];
    TaskHdl current = co_await GetCurrentTask();
    
    // Send multiple messages
    for (int i = 0; i < msgPerClient; i++) {
        // Prepare message
        int len = std::snprintf(buff, sizeof(buff), "Message %d from Task %d", i, taskId); 
        std::print("Client {} Received: {}\n", taskId, len); 
        // Send message
        result = co_await IOWrite(sock, buff, len, 0);
        if (result < 0) {
            std::print("Send failed\n");
            break;
        }


        // Receive echo
        result = co_await IORead(sock, buff, sizeof(buff)-1, 0);
        if (result < 0) {
            std::print("Receive failed\n");
            break;
        }
        buff[result] = '\0';
        
        std::print("Received: {}\n", buff);
    }

    co_await IOClose(sock);
}

DefineTask MainTask(int clientCount, int msgPerClient, const char* serverIp, int serverPort) noexcept {
    // Create array to store client task handles
    std::vector<TaskHdl> clients(clientCount);

    // Launch client tasks
    for (int i = 0; i < clientCount; i++) {
        clients[i] = ClientTask(i, serverIp, serverPort, msgPerClient);
    }

    // Wait for all clients to complete
    for (TaskHdl& client : clients) {
        co_await JoinTask(client);
    }
    std::print("All clients completed\n");
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
    return RunMain(&config, MainTask, clientCount, msgPerClient, serverIp, serverPort);
}