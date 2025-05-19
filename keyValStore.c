#include "keyValStore.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ENTRIES 100

typedef struct {
    char key[256];
    char value[256];
} Entry;

typedef struct {
    Entry entries[MAX_ENTRIES];
    int count;
    sem_t mutex;         // schützt Zugriff auf den Store
    sem_t tx_lock;       // schützt Transaktionen
    int in_transaction;  // Status
} SharedStore;

static SharedStore* store = NULL;

void init_store() {
    // Stelle sicher, dass Datei für ftok existiert
    const char* ftok_file = "/tmp/keyvalstore";
    int fd = open(ftok_file, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("open ftok file");
        exit(EXIT_FAILURE);
    }
    close(fd);

    key_t key = ftok(ftok_file, 65);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key, sizeof(SharedStore), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    store = (SharedStore*) shmat(shmid, NULL, 0);
    if (store == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Nur einmal initialisieren
    if (store->count == 0) {
        sem_init(&(store->mutex), 1, 1);
        sem_init(&(store->tx_lock), 1, 1);
        store->count = 0;
        store->in_transaction = 0;
    }
}

int begin_transaction() {
    sem_wait(&(store->tx_lock));
    store->in_transaction = 1;
    return 0;
}

int end_transaction() {
    store->in_transaction = 0;
    sem_post(&(store->tx_lock));
    return 0;
}

int is_transaction_active() {
    return store->in_transaction;
}

int put(char* key, char* value) {
    while (is_transaction_active()) {
        usleep(10000);
    }

    sem_wait(&(store->mutex));
    for (int i = 0; i < store->count; ++i) {
        if (strcmp(store->entries[i].key, key) == 0) {
            strncpy(store->entries[i].value, value, 255);
            sem_post(&(store->mutex));
            return 0;
        }
    }
    if (store->count < MAX_ENTRIES) {
        strncpy(store->entries[store->count].key, key, 255);
        strncpy(store->entries[store->count].value, value, 255);
        store->count++;
        sem_post(&(store->mutex));
        return 1;
    }
    sem_post(&(store->mutex));
    return -1;
}

int get(char* key, char* res) {
    while (is_transaction_active()) {
        usleep(10000);
    }

    sem_wait(&(store->mutex));
    for (int i = 0; i < store->count; ++i) {
        if (strcmp(store->entries[i].key, key) == 0) {
            strncpy(res, store->entries[i].value, 255);
            sem_post(&(store->mutex));
            return 0;
        }
    }
    sem_post(&(store->mutex));
    return -1;
}

int del(char* key) {
    while (is_transaction_active()) {
        usleep(10000);
    }

    sem_wait(&(store->mutex));
    for (int i = 0; i < store->count; ++i) {
        if (strcmp(store->entries[i].key, key) == 0) {
            store->entries[i] = store->entries[store->count - 1];
            store->count--;
            sem_post(&(store->mutex));
            return 0;
        }
    }
    sem_post(&(store->mutex));
    return -1;
}
