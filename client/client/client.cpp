#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

#define BUFFER_SIZE 1024


int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to SSH Server at %s:%d\n", ip, port);

    while (1) {
        printf("Enter command (type 'exit' to quit): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        send(clientSocket, buffer, strlen(buffer), 0);

        ZeroMemory(buffer, BUFFER_SIZE);

        int recvSize = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (recvSize <= 0) {
            printf("Server disconnected or error.\n");
            break;
        }

        buffer[recvSize] = '\0';
        printf("Server Response:\n%s\n", buffer);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}