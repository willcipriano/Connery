#include <string.h>
#include <stdlib.h>
#include "cval.h"
#include "hashtable.h"


unsigned int hash(const char *key, const long table_size) {
    unsigned long int value = 0;
    unsigned int key_len = strlen(key);

    for (unsigned int i = 0; i < key_len; ++i) {
        value = value * 37 + key[i];
    }

    value = value % table_size;

    return value;
}

hash_table_entry* hash_table_pair(const char *key, cval* value) {
    hash_table_entry *entry = malloc(sizeof(hash_table_entry) * 1);

    entry->key = malloc(strlen(key) + 1);
    strcpy(entry->key, key);

    entry->value = malloc(sizeof(cval));
    entry->value = cval_copy(value);

    entry->next = NULL;
    return entry;
}

void hash_table_set(hash_table* target_hash_table, const char* key, cval* value) {
    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry* entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        target_hash_table->entries[slot] = hash_table_pair(key, value);
        target_hash_table->items += 1;
        return;
    }

    hash_table_entry* prev;

    while (entry != NULL) {

        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = malloc(sizeof(cval));
            entry->value = cval_copy(value);
            return;
        }

        prev = entry;
        entry = prev->next;
    }

    prev->next = hash_table_pair(key, value);
    target_hash_table->items += 1;
}

cval* hash_table_get(hash_table* target_hash_table, const char* key) {
    if (target_hash_table->items == 0) {
        return NULL;
    }

    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry* entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        return NULL;
    }

    while (entry != NULL) {

        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }

        entry = entry->next;
    }

    return NULL;
}

hash_table* hash_table_create(const long table_size) {

    hash_table *ht = malloc(sizeof(hash_table) * 1);
    ht->table_size = table_size;
    ht->entries = malloc(sizeof(hash_table_entry*) * table_size);

    for (int i = 0; i < table_size; ++i) {
        ht->entries[i] = NULL;
    }

    return ht;
}

void hash_table_entry_delete(hash_table* target_hash_table, const char *key) {
    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry* entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        return ;
    }

    hash_table_entry* prev;
    int idx = 0;

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {

            if (entry->next == NULL && idx == 0) {
                target_hash_table->entries[slot] = NULL;
            }

            if (entry->next != NULL && idx == 0) {
                target_hash_table->entries[slot] = entry->next;
            }

            if (entry->next == NULL && idx != 0) {
                prev->next = NULL;
            }

            if (entry->next != NULL && idx != 0) {
                prev->next = entry->next;
            }

            free(entry->key);
            cval_delete(entry->value);
            free(entry);
            target_hash_table->items -= 1;

            return;
        }

        prev = entry;
        entry = prev->next;

        ++idx;
    }
}

void hash_table_destroy(hash_table* target_hash_table) {

    for (long i = 0; i < target_hash_table->table_size; i++) {
        if (target_hash_table->entries[i] != NULL) {
            hash_table_entry_delete(target_hash_table, target_hash_table->entries[i]->key);
        }
    }

    free(target_hash_table);
}

hash_table* hash_table_copy(hash_table* target_hash_table) {
    hash_table* new_hash_table = hash_table_create(target_hash_table->table_size);

    for (long i = 0; i < target_hash_table->table_size; i++) {
        new_hash_table->entries[i] = target_hash_table->entries[i];
    }

    new_hash_table->items = target_hash_table->items;

    return new_hash_table;

}