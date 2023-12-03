#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

void error(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char message[1024];
    int bytes_received;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("WSAStartup failed");
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        error("Error creating socket");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        error("Connection failed");
    }

    printf("Connected to server. Type 'exit' to quit.\n");

    // Set the socket to non-blocking
    u_long non_blocking = 1;
    ioctlsocket(client_socket, FIONBIO, &non_blocking);

    while (1) {
        // Check if there's any message from the server
        bytes_received = recv(client_socket, message, sizeof(message), 0);
        if (bytes_received > 0) {
            message[bytes_received] = '\0'; // Add null terminator to make it a string
            printf("Received from server: %s", message);
        }

        // Read user input from the console
        fgets(message, sizeof(message), stdin);

        // Check if the user wants to exit
        if (strcmp(message, "exit\n") == 0) {
            send(client_socket, message, strlen(message), 0);
            break;
        }

        // Send user input to server
        send(client_socket, message, strlen(message), 0);

        // Clear message buffer
        memset(message, 0, sizeof(message));
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}