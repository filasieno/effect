#include "ak.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <print>

using namespace ak;


DefineTask ClientTask(int task_id,const char* server_ip, int server_port, int messages_per_client) noexcept {
    // Create socket
    int sock = co_await XSocket(AF_INET, SOCK_STREAM, 0, 0);
    if (sock < 0) {
        std::print("Failed to create socket\n");
        co_return;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::print("Invalid address\n");
        co_await XClose(sock);
        co_return;
    }

    // Connect to server
    int result = co_await XConnect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
        std::print("Connection failed\n");
        co_await XClose(sock);
        co_return;
    }
    std::print("task {} connected to server\n", task_id);

    char buff[128];
    TaskHdl current = co_await GetCurrentTask();
    
    // Send multiple messages
    for (int i = 0; i < messages_per_client; i++) {
        // Prepare message
        int len = std::snprintf(buff, sizeof(buff), "Message %d from Task %d", i, task_id); 
        std::print("Client {} Received: {}\n", task_id, len); 
        // Send message
        result = co_await XWrite(sock, buff, len, 0);
        if (result < 0) {
            std::print("Send failed\n");
            break;
        }


        // Receive echo
        result = co_await XRead(sock, buff, sizeof(buff)-1, 0);
        if (result < 0) {
            std::print("Receive failed\n");
            break;
        }
        buff[result] = '\0';
        
        std::print("Received: {}\n", buff);
    }

    co_await XClose(sock);
}

DefineTask MainTask(int client_count, int messages_per_client, const char* server_ip, int server_port) noexcept {
    // Create array to store client task handles
    std::vector<TaskHdl> clients(client_count);

    // Launch client tasks
    for (int i = 0; i < client_count; i++) {
        clients[i] = ClientTask(i, server_ip, server_port, messages_per_client);
    }

    // Wait for all clients to complete
    for (TaskHdl& client : clients) {
        co_await client;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::print("Usage: {} <server_ip> <server_port> <client_count> <messages_per_client>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int server_port = std::atoi(argv[2]);
    int client_count = std::atoi(argv[3]);
    int messages_per_client = std::atoi(argv[4]);

    // Configure kernel
    KernelConfig config{
        .mem = nullptr,
        .memSize = 0,
        .ioEntryCount = 1024
    };

    // Run the main task
    return RunMain(&config, MainTask, client_count, messages_per_client, server_ip, server_port);
}