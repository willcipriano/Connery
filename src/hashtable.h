#ifndef CONNERY_HASHTABLE_H
#define CONNERY_HASHTABLE_H

typedef struct cval cval;

typedef struct hash_table_entry {
    char *key;
    cval *value;
    struct hash_table_entry *next;
} hash_table_entry;

typedef struct {
    long table_size;
    long items;
    hash_table_entry **entries;
} hash_table;

hash_table *hash_table_create(long table_size);

void hash_table_set(hash_table *target_hash_table, const char *key, cval *value);

cval *hash_table_get(hash_table *target_hash_table, const char *key);

void hash_table_destroy(hash_table *target_hash_table);

hash_table *hash_table_copy(hash_table *target_hash_table);

hash_table *hash_table_copy_and_resize(hash_table *target_hash_table, long newSize);

void hash_table_entry_delete(hash_table *target_hash_table, const char *key);

int hash_table_print(hash_table *target_hash_table);

cval **hash_table_dump_values(hash_table *target_hash_table);

cval **hash_table_dump_keys(hash_table *target_hash_table);

#endif //CONNERY_HASHTABLE_H
