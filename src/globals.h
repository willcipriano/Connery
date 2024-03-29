#ifndef CONNERY_GLOBALS_H
#define CONNERY_GLOBALS_H

// system
#define SYSTEM_LANG 0
#define CONNERY_VERSION "0.0.3"
#define CONNERY_VER_INT 3
#define LOG_LEVEL 4
#define TRACE_ENABLED 1
#define STD_LIB_LOCATION "stdlib/main.connery"
#define ENV_HASH_TABLE_SIZE 100

// allocator
#define PREALLOCATE_SLOTS 4096
#define PREALLOCATE_ROWS 16
#define ROWS_MAX 4194304
#define MAX_OBJECT_ID 17179869184
#define PRE_CACHE_SIZE 16

// hash tables
#define HASH_TABLE_RESIZE_DEPTH 3
#define HASH_TABLE_RESIZE_MULTIPLIER 2
#define HASH_TABLE_RESIZE_BONUS 100
#define DICTIONARY_LITERAL_INSTANTIATED_HASH_TABLE_MINIMUM 25

// http
#define COOKIE_JAR "connery.cookies"

//json
#define JSON_OBJECT_HT_INIT_SIZE 25
#define JSON_STRING_DEFAULT_OUTPUT_MODE JSON_C_TO_STRING_PRETTY

// hashing
#define HASHING_SEED 1234567

#endif //CONNERY_GLOBALS_H
