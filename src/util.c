#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cval.h"
#include "util.h"
#include "globals.h"
#include "murmurhash.h"


typedef char *multi_tok_t;

char *multi_tok(char *input, multi_tok_t *string, char *delimiter) {
    if (input != NULL)
        *string = input;

    if (*string == NULL)
        return *string;

    char *end = strstr(*string, delimiter);
    if (end == NULL) {
        char *temp = *string;
        *string = NULL;
        return temp;
    }

    char *temp = *string;

    *end = '\0';
    *string = end + strlen(delimiter);
    return temp;
}

multi_tok_t multiTok_init() { return NULL; }

long long_power(long x,long exponent)
{
    int i;
    int number = 1;
    for (i = 0; i < exponent; ++i)
        number *= x;
    return(number);
}

int count_digits(long n)
{
    if (n == 0)
        return 0;
    return 1 + count_digits(n / 10);
}

long get_factor(int init_digits) {

    switch (init_digits) {
        case 2:
            return 10;
        case 3:
            return 100;
        case 4:
            return 1000;
        case 5:
            return 10000;
        case 6:
            return 100000;
        case 7:
            return 1000000;
        case 8:
            return 10000000;
        case 9:
            return 100000000;
        case 10:
            return 1000000000;
        case 11:
            return 10000000000;
        case 12:
            return 100000000000;
        case 13:
            return 1000000000000;
        case 14:
            return 10000000000000;
        case 15:
            return 100000000000000;
        case 16:
            return 1000000000000000;
        case 17:
            return 10000000000000000;
        case 18:
            return 100000000000000000;
        case 19:
            return 1000000000000000000;
        default:
            return -1;
    }
}

uint32_t murmurhash_cval_impl(cval* value,uint32_t seed) {
    cval ** temp_values = NULL;
    char temp_str[256];
    int cur = 0;
    switch(value->type) {
        case CVAL_S_EXPRESSION:
        case CVAL_Q_EXPRESSION:
            while (cur < value->count) {
                seed = murmurhash_cval_impl(value->cell[cur], seed);
                cur += 1;
            }
            return seed;

        case CVAL_DICTIONARY:
            temp_values = hash_table_dump_keys(value->ht);

            while ((value->ht->items - 1) > cur) {
                seed = murmurhash_cval_impl(temp_values[cur], seed);
                seed = murmurhash_cval_impl(hash_table_get(value->ht, temp_values[cur]->str), seed);
                cur += 1;
            }

            return seed;

        case CVAL_STRING:
            return murmurhash(value->str, (uint32_t) strlen(value->str), seed);

        case CVAL_NUMBER:
            sprintf(temp_str, "%ld", value->num);
            return murmurhash(temp_str, (uint32_t) strlen(temp_str), seed);

        case CVAL_FLOAT:
            sprintf(temp_str, "%Lg", value->fnum);
            return murmurhash(temp_str, (uint32_t) strlen(temp_str), seed);

        case CVAL_BOOLEAN:
            if (value->boolean) {
                return murmurhash("True", (uint32_t) 4, seed);
            }
            return murmurhash("False", (uint32_t) 5, seed);

        case CVAL_NULL:
            return murmurhash("Null", (uint32_t) 4, seed);

        case CVAL_SYMBOL:
            return murmurhash(value->sym, (uint32_t) strlen(value->sym), seed);

        case CVAL_FAULT:
            return murmurhash(value->err, (uint32_t) strlen(value->err), seed);

        default:
            return seed;

    }
}

cval *murmurhash_cval(cval* value) {
    return cval_number((long) murmurhash_cval_impl(value, (uint32_t) HASHING_SEED));
}

cval *murmurhash_cval_with_seed(cval* value, cval* seed) {
    return cval_number((long) murmurhash_cval_impl(value, (uint32_t) seed->num));
}



