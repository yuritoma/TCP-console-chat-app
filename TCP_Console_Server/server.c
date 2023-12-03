#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define REMOTE_SERVER_IP "127.0.0.1"
#define REMOTE_SERVER_PORT 9090
#define USERNAME "admin"
#define PASSWORD "password"

SOCKET client_sockets[MAX_CLIENTS];
int client_ids[MAX_CLIENTS];
SOCKET remote_server_socket;
HANDLE threads[MAX_CLIENTS];

void error(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void closeServer() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != INVALID_SOCKET) {
            closesocket(client_sockets[i]);
        }
    }
    closesocket(remote_server_socket);
    WSACleanup();
    exit(0);
}

DWORD WINAPI clientHandler(LPVOID socket_ptr) {
    SOCKET client_socket = *(SOCKET*)socket_ptr;
    int client_id;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket) {
            client_id = client_ids[i];
            break;
        }
    }

    char message[1024];

    while (1) {
        int bytes_received = recv(client_socket, message, sizeof(message), 0);

        if (bytes_received <= 0) {
            closesocket(client_socket);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == client_socket) {
                    client_sockets[i] = INVALID_SOCKET;
                    break;
                }
            }

            printf("Client %d disconnected\n", client_id);
            return 0;
        }

        message[bytes_received] = '\0';
        printf("Client %d: %s", client_id, message);

        // Transmit the message to the remote server
        send(remote_server_socket, message, strlen(message), 0);

        // Broadcast the message to all other connected clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != INVALID_SOCKET && client_sockets[i] != client_socket) {
                send(client_sockets[i], message, strlen(message), 0);
            }
        }
    }
}

BOOL CtrlHandler(DWORD fdwCtrlType) {
    if (fdwCtrlType == CTRL_C_EVENT) {
        printf("Ctrl+C received. Closing server.\n");

        // Close client sockets and the remote server socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != INVALID_SOCKET) {
                closesocket(client_sockets[i]);
            }
        }

        closesocket(remote_server_socket);

        // Wait for the threads to finish before exiting the program
        WaitForMultipleObjects(MAX_CLIENTS, threads, TRUE, INFINITE);

        // Clean up WinSock
        WSACleanup();

        // Exit the program
        exit(0);
        return TRUE;
    }

    return FALSE;
}

int signInToRemoteServer(SOCKET remote_server_socket) {
    char username[256], password[256];
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    char signInMessage[1024];
    sprintf(signInMessage, "SIGNIN %s %s\n", username, password);

    // Send sign-in message to remote server
    send(remote_server_socket, signInMessage, strlen(signInMessage), 0);

    char response[1024];
    int bytes_received = recv(remote_server_socket, response, sizeof(response), 0);

    if (bytes_received <= 0) {
        printf("Error receiving sign-in response from remote server\n");
        return 0;
    }

    response[bytes_received] = '\0';

    if (strcmp(response, "ACCEPT\n") == 0) {
        printf("Sign-in successful\n");
        return 1;
    }
    else {
        printf("Sign-in denied. Exiting...\n");
        return 0;
    }
}


int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
        error("Error setting Ctrl+C handler");
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("WSAStartup failed");
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        error("Error creating socket");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        error("Bind failed");
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        error("Listen failed");
    }

    printf("Server started. Waiting for connections...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_ids[i] = i + 1;
    }

    remote_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_server_socket == INVALID_SOCKET) {
        error("Error creating remote server socket");
    }

    struct sockaddr_in remote_server_addr;
    remote_server_addr.sin_family = AF_INET;
    remote_server_addr.sin_addr.s_addr = inet_addr(REMOTE_SERVER_IP);
    remote_server_addr.sin_port = htons(REMOTE_SERVER_PORT);

    if (connect(remote_server_socket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) == SOCKET_ERROR) {
        error("Error connecting to remote server");
    }

    if (!signInToRemoteServer(remote_server_socket)) {
        closesocket(remote_server_socket);
        WSACleanup();
        return 1;
    }

    int client_count = 0;

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);

        if (client_socket == INVALID_SOCKET) {
            error("Accept failed");
        }

        printf("Client %d connected\n", client_ids[client_count]);

        client_sockets[client_count] = client_socket;
        threads[client_count] = CreateThread(NULL, 0, clientHandler, (LPVOID)&client_sockets[client_count], 0, NULL);
        client_count++;

        if (client_count >= MAX_CLIENTS) {
            // Wait for the threads to finish before accepting new connections
            WaitForMultipleObjects(MAX_CLIENTS, threads, TRUE, INFINITE);
            client_count = 0;
        }
    }

    return 0;
}