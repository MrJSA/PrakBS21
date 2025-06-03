#include "sub.h"
#include "keyValStore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS  FD_SETSIZE

typedef struct Subscription {
    char key[256];
    int client_fd;
    struct Subscription* next;
} Subscription;

static Subscription* subscription_head = NULL;

void subscribe(const char* key, int client_fd) {
    Subscription* sub = malloc(sizeof(Subscription));
    if (!sub) return;
    strncpy(sub->key, key, sizeof(sub->key) - 1);
    sub->client_fd = client_fd;
    sub->next = subscription_head;
    subscription_head = sub;
}

void notify_subscribers(const char* key, const char* message, int excluding_fd) {
    Subscription* sub = subscription_head;
    while (sub != NULL) {
        if (strcmp(sub->key, key) == 0 && sub->client_fd != excluding_fd) {
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), "> %s", message);
            send(sub->client_fd, msg, strlen(msg), 0);
        }
        sub = sub->next;
    }
}

void handle_command(int client_fd, char* buffer, int* in_transaction) {
    char cmd[10], key[256], value[256], response[BUFFER_SIZE];
    memset(cmd, 0, sizeof(cmd));
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));
    sscanf(buffer, "%s %s %s", cmd, key, value);

    if ((strcmp(cmd, "PUT") == 0 || strcmp(cmd, "GET") == 0 || strcmp(cmd, "DEL") == 0)
        && !(*in_transaction) && is_transaction_active()) {
        snprintf(response, sizeof(response), "> Store locked by another transaction. Please wait.\n");
        send(client_fd, response, strlen(response), 0);
        return;
    }

    if (strcmp(cmd, "PUT") == 0) {
        put(key, value);
        snprintf(response, sizeof(response), "> PUT:%s:%s\n", key, value);
        send(client_fd, response, strlen(response), 0);
        notify_subscribers(key, response + 2, client_fd); // strip '> '
    } else if (strcmp(cmd, "GET") == 0) {
        if (get(key, value) == 0)
            snprintf(response, sizeof(response), "> GET:%s:%s\n", key, value);
        else
            snprintf(response, sizeof(response), "> GET:%s:key_nonexistent\n", key);
        send(client_fd, response, strlen(response), 0);
    } else if (strcmp(cmd, "DEL") == 0) {
        if (del(key) == 0)
            snprintf(response, sizeof(response), "> DEL:%s:key_deleted\n", key);
        else
            snprintf(response, sizeof(response), "> DEL:%s:key_nonexistent\n", key);
        send(client_fd, response, strlen(response), 0);
        notify_subscribers(key, response + 2, client_fd);
    } else if (strcmp(cmd, "BEG") == 0) {
        begin_transaction();
        *in_transaction = 1;
        snprintf(response, sizeof(response), "> TRANSACTION STARTED\n");
        send(client_fd, response, strlen(response), 0);
    } else if (strcmp(cmd, "END") == 0) {
        if (*in_transaction) {
            end_transaction();
            *in_transaction = 0;
            snprintf(response, sizeof(response), "> TRANSACTION ENDED\n");
        } else {
            snprintf(response, sizeof(response), "> No active transaction\n");
        }
        send(client_fd, response, strlen(response), 0);
    } else if (strcmp(cmd, "SUB") == 0) {
        subscribe(key, client_fd);
        char current_val[256] = {0};
        get(key, current_val);
        snprintf(response, sizeof(response), "> SUB:%s:%s\n", key, current_val);
        send(client_fd, response, strlen(response), 0);
    } else if (strcmp(cmd, "QUIT") == 0) {
        snprintf(response, sizeof(response), "> Goodbye\n");
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
    } else {
        snprintf(response, sizeof(response), "> Unknown command\n");
        send(client_fd, response, strlen(response), 0);
    }
}

void run_server(int port) {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS];
    int client_transaction[MAX_CLIENTS] = {0}; // 0 = no tx, 1 = tx active
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    fd_set read_fds, master_fds;
    char buffer[BUFFER_SIZE];
    int max_sd;

    for (int i = 0; i < MAX_CLIENTS; i++)
        client_sockets[i] = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    printf("Server listening on port %d\n", port);

    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_sd = server_fd;

    while (1) {
        read_fds = master_fds;
        select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    FD_SET(new_socket, &master_fds);
                    client_sockets[new_socket] = 1;
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New connection: socket %d\n", new_socket);
                } else {
                    int valread = read(i, buffer, BUFFER_SIZE - 1);
                    if (valread <= 0) {
                        close(i);
                        FD_CLR(i, &master_fds);
                        client_sockets[i] = 0;
                        client_transaction[i] = 0;
                        printf("Client disconnected: socket %d\n", i);
                    } else {
                        buffer[valread] = '\0';
                        handle_command(i, buffer, &client_transaction[i]);
                    }
                }
            }
        }
    }
}
