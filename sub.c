#include "sub.h"
#include "keyValStore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void run_server(int port) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        while (1) {
            int valread = read(new_socket, buffer, 1024);
            if (valread <= 0) break;
            buffer[valread] = '\0';

            char cmd[10], key[256], value[256] = {0};
            sscanf(buffer, "%s %s %s", cmd, key, value);

            if (strcmp(cmd, "QUIT") == 0) {
                send(new_socket, "Goodbye\n", strlen("Goodbye\n"), 0);
                break;
            } else if (strcmp(cmd, "PUT") == 0) {
                put(key, value);
                snprintf(buffer, sizeof(buffer), "PUT:%s:%s\n", key, value);
            } else if (strcmp(cmd, "GET") == 0) {
                if (get(key, value) == 0)
                    snprintf(buffer, sizeof(buffer), "GET:%s:%s\n", key, value);
                else
                    snprintf(buffer, sizeof(buffer), "GET:%s:key_nonexistent\n", key);
            } else if (strcmp(cmd, "DEL") == 0) {
                if (del(key) == 0)
                    snprintf(buffer, sizeof(buffer), "DEL:%s:key_deleted\n", key);
                else
                    snprintf(buffer, sizeof(buffer), "DEL:%s:key_nonexistent\n", key);
            } else {
                snprintf(buffer, sizeof(buffer), "Unknown command\n");
            }
            send(new_socket, buffer, strlen(buffer), 0);
        }
        close(new_socket);
    }
}
