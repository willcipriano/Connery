#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

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


