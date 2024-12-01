#include <winsock.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

#define BUFFER_SIZE 1024

#define _WINSOCK_DEPRECATED_NO_WARNINGS


DWORD WINAPI handle_client(LPVOID lpParam);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip);
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("SSH Server: Listening on %s:%d...\n", ip, port);

    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("SSH Server: Client connected.\n");

    HANDLE hThread = CreateThread(NULL, 0, handle_client, (LPVOID)(SOCKET)clientSocket, 0, NULL);
    if (hThread == NULL) {
        printf("CreateThread failed (%d).\n", GetLastError());
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

DWORD WINAPI handle_client(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    char currentDirectory[MAX_PATH];

    if (!GetCurrentDirectoryA(MAX_PATH, currentDirectory)) {
        snprintf(currentDirectory, MAX_PATH, "C:\\");
    }

    while (1) {
        ZeroMemory(buffer, BUFFER_SIZE);

        int recvSize = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (recvSize <= 0) {
            printf("Client disconnected or error.\n");
            break;
        }

        buffer[recvSize] = '\0';
        printf("Client Command: %s\n", buffer);

        char response[BUFFER_SIZE * 2];

        if (strncmp(buffer, "cd ", 3) == 0) {
            char* path = buffer + 3;
            if (SetCurrentDirectoryA(path)) {
                GetCurrentDirectoryA(MAX_PATH, currentDirectory);
                snprintf(response, sizeof(response), "Changed directory to: %s\n", currentDirectory);
            }
            else {
                snprintf(response, sizeof(response), "Error: Could not change directory to '%s'.\n", path);
            }
        }
        else if (strcmp(buffer, "cd") == 0) {
            snprintf(response, sizeof(response), "Current directory: %s\n", currentDirectory);
        }
        else {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "cd \"%s\" && %s", currentDirectory, buffer);

            FILE* pipe = _popen(command, "r");
            if (!pipe) {
                snprintf(response, sizeof(response), "Error: Failed to execute command.\n");
            }
            else {
                size_t len = fread(response, 1, sizeof(response) - 1, pipe);
                response[len] = '\0';
                _pclose(pipe);
            }
        }

        send(clientSocket, response, strlen(response), 0);
    }

    closesocket(clientSocket);
    return 0;
}