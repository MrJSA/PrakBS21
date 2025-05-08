#include "keyValStore.h"
#include <string.h>
#include <stdlib.h>

typedef struct KeyValue {
    char key[256];
    char value[256];
    struct KeyValue *next;
} KeyValue;

static KeyValue *head = NULL;

int put(char* key, char* value) {
    KeyValue *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(curr->value, value, 255);
            return 0;
        }
        curr = curr->next;
    }
    KeyValue *new = malloc(sizeof(KeyValue));
    strncpy(new->key, key, 255);
    strncpy(new->value, value, 255);
    new->next = head;
    head = new;
    return 1;
}

int get(char* key, char* res) {
    KeyValue *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(res, curr->value, 255);
            return 0;
        }
        curr = curr->next;
    }
    return -1;
}

int del(char* key) {
    KeyValue **indirect = &head;
    while (*indirect != NULL) {
        if (strcmp((*indirect)->key, key) == 0) {
            KeyValue *tmp = *indirect;
            *indirect = tmp->next;
            free(tmp);
            return 0;
        }
        indirect = &(*indirect)->next;
    }
    return -1;
}
