#ifndef CONNERY_HASHTABLE_H
#define CONNERY_HASHTABLE_H
#include "cval.h"

typedef struct hash_table_entry {
    char* key;
    cval* value;
    struct hash_table_entry *next;
} hash_table_entry;

typedef struct {
    long table_size;
    hash_table_entry **entries;
} hash_table;

hash_table* hash_table_create(long table_size);

#endif //CONNERY_HASHTABLE_H
