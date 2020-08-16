//
// Created by dev on 8/15/20.
//

#ifndef CONNERY_UTIL_H
#define CONNERY_UTIL_H

struct http_response {
    char *body;
    size_t len;
};
void init_http_response(struct http_response *s);
size_t http_response_writer(void *ptr, size_t size, size_t nmemb, struct http_response *s);
typedef char *multi_tok_t;
char *multi_tok(char *input, multi_tok_t *string, char *delimiter);
multi_tok_t multiTok_init();
char *concatenateThree(const char *a, const char *b, const char *c);

#endif //CONNERY_UTIL_H
