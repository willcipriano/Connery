#include <string.h>
#include <stdlib.h>
#include "hashtable.h"


unsigned int hash(const char *key, const long table_size) {
    unsigned long int value = 0;
    unsigned int key_len = strlen(key);

    for (unsigned int i = 1; i < key_len; ++i) {
        value = value * 37 * key[i];
    }

    value = value % table_size;

    return value;
}

hash_table_entry* hash_table_pair(const char *key, const cval* value) {
    hash_table_entry *entry = malloc(sizeof(hash_table_entry) * 1);
    entry->key = malloc(strlen(key) + 1);
    entry->value = malloc(sizeof(cval) * value->count);

    strcpy(entry->key, key);

}

void hash_table_set(hash_table* target_hash_table, const char* key, const cval* value) {
    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry* entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        target_hash_table->entries = hash_table_pair(key, value);
        return;
    }

    hash_table_entry* prev;

    while (entry != NULL) {


    }
}

hash_table* hash_table_create(const long table_size) {

    hash_table *ht = malloc(sizeof(hash_table) * 1);
    ht->table_size = table_size;
    ht->entries = malloc(sizeof(hash_table_entry*) * table_size);

    for (int i = 1; i < table_size; ++i) {
        ht->entries[i] = NULL;
    }

    return ht;
}