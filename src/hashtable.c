#include <string.h>
#include <stdlib.h>
#include "cval.h"
#include "murmurhash.h"
#include "hashtable.h"
#include "allocator.h"
#include "globals.h"

unsigned int hash(const char *key, const long table_size) {
    uint32_t hash = murmurhash(key, (uint32_t) strlen(key), (uint32_t) HASHING_SEED);
    return (long) hash % table_size;
}

hash_table_entry *hash_table_pair(const char *key, cval *value) {
    hash_table_entry *entry = malloc(sizeof(hash_table_entry));

    entry->key = malloc(strlen(key) + 1);

    strcpy(entry->key, key);

    entry->value = cval_copy(value);

    entry->next = NULL;
    return entry;
}



void hash_table_set(hash_table *target_hash_table, const char *key, cval *value) {
    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry *entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        target_hash_table->entries[slot] = hash_table_pair(key, value);

        target_hash_table->items += 1;
        return;
    }

    hash_table_entry *prev;
    int depth = 1;

    while (entry != NULL) {

        if (strcmp(entry->key, key) == 0) {
            (entry->value);
            entry->value = allocate();
            entry->value = cval_copy(value);
            return;
        }

        prev = entry;
        entry = prev->next;
        depth += 1;
    }

    prev->next = hash_table_pair(key, value);
    target_hash_table->items += 1;

    if (depth >= HASH_TABLE_RESIZE_DEPTH) {
        hash_table *ht;
        ht = hash_table_copy_and_resize(target_hash_table, (target_hash_table->table_size * HASH_TABLE_RESIZE_MULTIPLIER) + HASH_TABLE_RESIZE_BONUS);
        target_hash_table->entries = ht->entries;
        target_hash_table->table_size = ht->table_size;
        target_hash_table->items = ht->items;

        ht->entries = NULL;
        hash_table_destroy(ht);
    }
}


cval *hash_table_get(hash_table *target_hash_table, const char *key) {

    if (target_hash_table == NULL) {
        return NULL;
    }

    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry *entry = target_hash_table->entries[slot];

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

hash_table *hash_table_create(const long table_size) {

    hash_table *ht = calloc(sizeof(hash_table), 1);
    ht->table_size = table_size;
    ht->entries = calloc(sizeof(hash_table_entry*), table_size);
    ht->items = 0;

    for (int i = 0; i < table_size; ++i) {
            ht->entries[i] = NULL;
    }

    return ht;
}

void hash_table_entry_delete(hash_table *target_hash_table, const char *key) {
    unsigned int slot = hash(key, target_hash_table->table_size);

    hash_table_entry *entry = target_hash_table->entries[slot];

    if (entry == NULL) {
        return;
    }

    hash_table_entry *prev;
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
            target_hash_table->items -= 1;

            return;
        }

        prev = entry;
        entry = prev->next;

        ++idx;
    }
}

void hash_table_destroy(hash_table *target_hash_table) {

    if (target_hash_table->entries == NULL) {
        free(target_hash_table->entries);
        free(target_hash_table);
        return;
    }

    for (long i = 0; i < target_hash_table->table_size; i++) {
        if (target_hash_table->entries[i] != NULL) {
            hash_table_entry_delete(target_hash_table, target_hash_table->entries[i]->key);
        }
    }

    free(target_hash_table->entries);
    free(target_hash_table);
}


hash_table *hash_table_copy_and_resize(hash_table *target_hash_table, long newSize) {

    hash_table *new_hash_table = NULL;

    if (newSize == 0) {
    new_hash_table = hash_table_create(target_hash_table->table_size); }
    else {
        new_hash_table = hash_table_create(newSize);
    }


    if (target_hash_table->items == 0) {
        return new_hash_table;
    }

    for (long i = 0; i < target_hash_table->table_size; i++) {
        if (target_hash_table->entries[i] != NULL) {
            hash_table_set(new_hash_table, target_hash_table->entries[i]->key, cval_copy(target_hash_table->entries[i]->value));

            hash_table_entry* prev = target_hash_table->entries[i]->next;

            while (prev != NULL) {
                hash_table_set(new_hash_table, prev->key, cval_copy(prev->value));
                prev = prev->next;
            }
        }
    }

    return new_hash_table;
}

hash_table *hash_table_copy(hash_table *target_hash_table) {
    return hash_table_copy_and_resize(target_hash_table, 0);
}

cval **hash_table_dump_values(hash_table *target_hash_table) {
    cval** array = calloc(sizeof(cval*),target_hash_table->items);
    int itemsFound = 0;

    for (long i = 0; i < target_hash_table->table_size; i++) {

        if (target_hash_table->entries[i] != NULL) {
            itemsFound += 1;
            array[itemsFound - 1] = target_hash_table->entries[i]->value;

            if (target_hash_table->entries[i]->next != NULL) {
                hash_table_entry *entry = target_hash_table->entries[i]->next;
                while (entry != NULL) {
                    itemsFound += 1;
                    array[itemsFound - 1] = entry->value;
                    entry = entry->next;
                }
            }
        }
    }

    return array;
}

cval **hash_table_dump_keys(hash_table *target_hash_table) {
    cval** array = calloc(sizeof(cval*), target_hash_table->items);
    int itemsFound = 0;

    for (long i = 0; i < target_hash_table->table_size; i++) {
        if (target_hash_table->entries[i] != NULL) {
            itemsFound += 1;
            array[itemsFound - 1] = cval_string(target_hash_table->entries[i]->key);

            if (target_hash_table->entries[i]->next != NULL) {
                hash_table_entry *entry = target_hash_table->entries[i]->next;
                while (entry != NULL) {
                    itemsFound += 1;
                    array[itemsFound - 1] = cval_string(entry->key);
                    entry = entry->next;
                }
            }


        }
        if (target_hash_table->items == itemsFound) {
            return array;
        }
    }
}

int hash_table_print(hash_table *target_hash_table) {
    int first_row = 1;

    for (long i = 0; i < target_hash_table->table_size; i++) {

        hash_table_entry *cur = target_hash_table->entries[i];

        while (cur != NULL) {

            if (!first_row) {
                putchar('\n');
            } else {
                first_row = 0;
            }

            switch (cur->value->type) {

                case CVAL_STRING:
                    cval_print_ht_str(cur->value, cur->key);
                    break;

                case CVAL_S_EXPRESSION:
                    cval_expr_ht_print(cur->value, '(', ')', cur->key);
                    break;

                case CVAL_Q_EXPRESSION:
                    cval_expr_ht_print(cur->value, '{', '}', cur->key);
                    break;

                case CVAL_FUNCTION:
                    if (cur->value->builtin) {
                        printf("%s : <builtin>", cur->key);
                    } else {
                        printf("%s : (\\ ", cur->key);
                        cval_print(cur->value->formals);
                        putchar(' ');
                        cval_print(cur->value->body);
                        putchar(')');
                    }
                    break;

                case CVAL_NUMBER:
                    printf("%s : %li", cur->key, cur->value->num);
                    break;

                case CVAL_FLOAT:
                    printf("%s : %Lg", cur->key, cur->value->fnum);
                    break;

                default:
                    printf("%s : %s", cur->key, ctype_name(cur->value->type));
                    break;

            }
            cur = cur->next;
        }

    }

}