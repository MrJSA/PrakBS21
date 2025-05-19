#ifndef KEYVALSTORE_H
#define KEYVALSTORE_H

void init_store();

int put(char* key, char* value);
int get(char* key, char* res);
int del(char* key);

int begin_transaction();
int end_transaction();
int is_transaction_active();

#endif
