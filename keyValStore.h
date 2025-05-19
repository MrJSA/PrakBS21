#ifndef KEYVALSTORE_H
#define KEYVALSTORE_H

void init_store();
int put(char* key, char* value);
int get(char* key, char* res);
int del(char* key);

#endif
