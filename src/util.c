#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <values.h>
#include <sys/stat.h>
#include <errno.h>
#include "util.h"

void init_http_response(struct http_response *s) {
    s->len = 0;
    s->body = malloc(s->len+1);
    if (s->body == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->body[0] = '\0';
}

size_t http_response_writer(void *ptr, size_t size, size_t nmemb, struct http_response *s)
{
    size_t new_len = s->len + size*nmemb;
    s->body = realloc(s->body, new_len+1);
    if (s->body == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->body+s->len, ptr, size*nmemb);
    s->body[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

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

char *concatenateThree(const char *a, const char *b, const char *c) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    size_t clen = strlen(c);
    char *res = malloc(alen + blen + clen + 1);
    if (res) {
        memcpy(res, a, alen);
        memcpy(res + alen, b, blen);
        memcpy(res + alen + blen, c, clen + 1);
    }
    return res;
}

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

int mkpath(const char *path, mode_t mode)
{
	char tmp[PATH_MAX];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp),"%s",path);
	len = strlen(tmp);
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++)
		if(*p == '/') {
			*p = 0;
			if (mkdir(tmp, mode) < 0 && errno != EEXIST)
				return -1;
			*p = '/';
		}
	if (mkdir(tmp, mode) < 0 && errno != EEXIST)
		return -1;
	return 1;
}
