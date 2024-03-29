#ifndef CONNERY_UTIL_H
#define CONNERY_UTIL_H
#include "cval.h"

//multitok
typedef char *multi_tok_t;
char *multi_tok(char *input, multi_tok_t *string, char *delimiter);
multi_tok_t multiTok_init();

//misc
char *concatenateThree(const char *a, const char *b, const char *c);
long long_power(long x,long exponent);
int count_digits(long n);
long get_factor(int init_digits);
char *replace_str(char *str, char *orig, char *rep);

//mkpath
int mkpath(const char *path, mode_t mode);

//hashing
cval *murmurhash_cval(cval* value);
cval *murmurhash_cval_with_seed(cval* value, cval* seed);



#endif //CONNERY_UTIL_H
