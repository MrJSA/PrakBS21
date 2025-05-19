#include "keyValStore.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#define MAX_ENTRIES 100

typedef struct {
    char key[256];
    char value[256];
} Entry;

typedef struct {
    Entry entries[MAX_ENTRIES];
    int count;
    sem_t mutex; // prozessübergreifender POSIX-Semaphor
} SharedStore;

static SharedStore *store = NULL;

void init_store() {
    key_t key = ftok("/tmp", 65); // stabile ID
    int shmid = shmget(key, sizeof(SharedStore), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    store = (SharedStore *)shmat(shmid, NULL, 0);
    if (store == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialisierung nur einmal (nicht beim Re-attach anderer Prozesse)
    if (store->count == 0) {
        sem_init(&(store->mutex), 1, 1); // shared between processes
        store->count = 0;
    }
}

int put(char* key, char* value) {
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
