#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define REMOTE_SERVER_PORT 9090
#define STOP_COMMAND "STOP"
#define USERNAME "admin"
#define PASSWORD "password"

void error(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int signIn(char* username, char* password) {
    if (strcmp(username, USERNAME) == 0 && strcmp(password, PASSWORD) == 0) {
        return 1; // Authentication successful
    }
    else {
        return 0; // Authentication failed
    }
}

int main() {
    WSADATA wsa;
    SOCKET remote_server_socket;
    struct sockaddr_in remote_server_addr;
    int addr_len = sizeof(remote_server_addr);
    int signal_received = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("WSAStartup failed");
    }

    if ((remote_server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        error("Error creating socket");
    }

    remote_server_addr.sin_family = AF_INET;
    remote_server_addr.sin_addr.s_addr = INADDR_ANY;
    remote_server_addr.sin_port = htons(REMOTE_SERVER_PORT);

    if (bind(remote_server_socket, (struct sockaddr*)&remote_server_addr, sizeof(remote_server_addr)) == SOCKET_ERROR) {
        error("Bind failed");
    }

    if (listen(remote_server_socket, 5) == SOCKET_ERROR) {
        error("Listen failed");
    }

    printf("Remote server started. Waiting for incoming messages and stop commands...\n");

    SOCKET client_socket;
    struct sockaddr_in client_addr;

    while (!signal_received) {
        client_socket = accept(remote_server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            error("Accept failed");
        }

        char message[1024];
        int bytes_received = recv(client_socket, message, sizeof(message), 0);
        if (bytes_received <= 0) {
            closesocket(client_socket);
            continue; // Skip processing empty or invalid messages
        }

        message[bytes_received] = '\0';

        // Check if it's a sign-in message
        if (strncmp(message, "SIGNIN ", 7) == 0) {
            char username[256], password[256];
            sscanf(message, "SIGNIN %s %s", username, password);

            if (signIn(username, password)) {
                send(client_socket, "ACCEPT\n", 7, 0);
            }
            else {
                send(client_socket, "DENY\n", 5, 0);
                closesocket(client_socket);
                continue; // Don't process further if sign-in fails
            }
        }
        else {
            // Process other messages here
        }

        // Process the message
        printf("Received message from the main server: %s\n", message);

        // If the message is the stop command, set the signal_received flag to true
        if (strcmp(message, STOP_COMMAND) == 0) {
            signal_received = 1;
        }

        closesocket(client_socket);
    }

    closesocket(remote_server_socket);
    WSACleanup();
    return 0;
}