#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
