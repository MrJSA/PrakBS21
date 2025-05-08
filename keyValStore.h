#ifndef KEYVALSTORE_H
#define KEYVALSTORE_H

int put(char* key, char* value);
int get(char* key, char* res);
int del(char* key);

#endif
