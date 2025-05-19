#include "sub.h"
#include "keyValStore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    char cmd[10], key[256], value[256], response[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) break;

        sscanf(buffer, "%s %s %s", cmd, key, value);

        if (strcmp(cmd, "PUT") == 0) {
            put(key, value);
            snprintf(response, sizeof(response), "PUT:%s:%s\n", key, value);
        } else if (strcmp(cmd, "GET") == 0) {
            if (get(key, value) == 0)
                snprintf(response, sizeof(response), "GET:%s:%s\n", key, value);
            else
                snprintf(response, sizeof(response), "GET:%s:key_nonexistent\n", key);
        } else if (strcmp(cmd, "DEL") == 0) {
            if (del(key) == 0)
                snprintf(response, sizeof(response), "DEL:%s:key_deleted\n", key);
            else
                snprintf(response, sizeof(response), "DEL:%s:key_nonexistent\n", key);
        } else if (strcmp(cmd, "BEG") == 0) {
            begin_transaction();
            snprintf(response, sizeof(response), "TRANSACTION STARTED\n");
        } else if (strcmp(cmd, "END") == 0) {
            end_transaction();
            snprintf(response, sizeof(response), "TRANSACTION ENDED\n");
        } else if (strcmp(cmd, "QUIT") == 0) {
            snprintf(response, sizeof(response), "Goodbye\n");
            send(client_socket, response, strlen(response), 0);
            break;
        } else {
            snprintf(response, sizeof(response), "Unknown command\n");
        }

        send(client_socket, response, strlen(response), 0);
    }

    close(client_socket);
    exit(0);
}

void run_server(int port) {
    signal(SIGCHLD, SIG_IGN);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
        } else {
            close(client_fd);
        }
    }
}
