#include "globals.h"
#include "http.h"
#include "util.h"
#include "strings.h"
#include <curl/curl.h>

struct http_response {
    char *body;
    size_t len;
};
void init_http_response(struct http_response *s);
size_t http_response_writer(void *ptr, size_t size, size_t nmemb, struct http_response *s);

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

cval * http_req_impl(cenv* env, cval* a) {
    hash_table *resHt = hash_table_create(10);
    hash_table *requestTimeHt = hash_table_create(10);
    hash_table *sizeHt = hash_table_create(10);
    hash_table *speedHt = hash_table_create(2);
    hash_table *ipHt = hash_table_create(2);
    hash_table *headerHt = hash_table_create(15);

    CURL *curl;
    CURLcode res;
    long response_code;
    cval *response_list = cval_q_expression();
    cval *headers_list = cval_q_expression();
    char *html_body;

    hash_table *options_ht = NULL;

    char *url = a->cell[0]->str;

    if (a->count > 1) {
        options_ht = a->cell[1]->ht;
    }

    curl = curl_easy_init();
    if (curl) {
        struct http_response s;
        init_http_response(&s);
        struct curl_slist *chunk = NULL;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, COOKIE_JAR);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, COOKIE_JAR);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_response_writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, concat(2,"Connery ", CONNERY_VERSION));
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

        cval* ht_dict = hash_table_get(options_ht, "headers");

        if (ht_dict != NULL) {
            cval** keys = hash_table_dump_keys(ht_dict->ht);

            for (int i = 0; i < ht_dict->ht->items; i++) {
                cval* value = hash_table_get(ht_dict->ht, keys[i]->str);
                if (value->type != CVAL_STRING) {
                    return cval_fault("Unable to parse header located at key %s, expected %s got %s.", keys[i], ctype_name(CVAL_STRING), ctype_name(value->type));
                }
                chunk = curl_slist_append(chunk, concat(3, keys[i], ":", value->str));
            }
        }

        cval* body_val = hash_table_get(options_ht, "body");
        if (body_val != NULL) {
            if (body_val->type != CVAL_STRING) {
                return cval_fault("Unable to parse request body, expected %s got %s.", ctype_name(CVAL_STRING), ctype_name(body_val->type));
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_val->str);
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            char * temp_char = NULL;
            long temp_long = -1;
            double temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            hash_table_set(resHt, "response code", cval_number(response_code));

            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &temp_char);
            if (temp_char != NULL) {
                hash_table_set(resHt, "url", cval_string(temp_char));
            }
            temp_char = NULL;

            // ip
            curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &temp_char);
            if (temp_char != NULL) {
                hash_table_set(ipHt, "remote", cval_string(temp_char));
            }
            temp_char = NULL;

            curl_easy_getinfo(curl, CURLINFO_LOCAL_IP, &temp_char);
            if (temp_char != NULL) {
                hash_table_set(ipHt, "local", cval_string(temp_char));
            }
            temp_char = NULL;
            hash_table_set(resHt, "ip", cval_dictionary(ipHt));

            // speed
            curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &temp_double);
            if (temp_double != -1) {
                hash_table_set(speedHt, "up", cval_float(temp_double)); }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &temp_double);
            if (temp_double != -1) {
                hash_table_set(speedHt, "down", cval_float(temp_double));
            }
            temp_double = -1;

            hash_table_set(resHt, "speed", cval_dictionary(speedHt));

            // size
            curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &temp_long);
            if (temp_long != -1) {
                hash_table_set(sizeHt, "download", cval_number(temp_long));
            }
            temp_long = -1;

            curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &temp_long);
            if (temp_long != -1) {
                hash_table_set(sizeHt, "upload", cval_number(temp_long));}
            temp_long = -1;

            curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &temp_long);
            if (temp_long != -1) {
                hash_table_set(sizeHt, "header", cval_number(temp_long));}
            temp_long = -1;

            curl_easy_getinfo(curl, CURLINFO_REQUEST_SIZE, &temp_long);
            if (temp_long != -1) {
                hash_table_set(sizeHt, "request", cval_number(temp_long));}
            temp_long = -1;

            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &temp_double);
            if (temp_double != -1) {
                hash_table_set(sizeHt, "content length", cval_float(temp_double));
            }
            temp_double = -1;

            hash_table_set(resHt, "size", cval_dictionary(sizeHt));

            // time
            curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "total", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "dns", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "connect", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "ssl", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "prestart", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "start", cval_float(temp_double));
            }
            temp_double = -1;

            curl_easy_getinfo(curl, CURLINFO_REDIRECT_TIME, &temp_double);
            if (temp_double != -1) {
                hash_table_set(requestTimeHt, "redirect", cval_float(temp_double));
            }
            temp_double = -1;



            hash_table_set(resHt, "time", cval_dictionary(requestTimeHt));

            char *response;
            response = malloc(strlen(s.body) + 1);
            strcpy(response, s.body);

            multi_tok_t y = multiTok_init();
            char *headers = multi_tok(response, &y, "\r\n\r\n");

            multi_tok_t x = multiTok_init();
            char *header = multi_tok(headers, &x, "\r\n");

            while (header != NULL) {
                if (strstr(header, ": ")) {
                    multi_tok_t q = multiTok_init();
                    char *key = multi_tok(header, &q, ": ");

                    hash_table_set(headerHt, key, cval_string(multi_tok(NULL, &q, ": ")));
                } else {
                    if (strstr(header, "HTTP/")) {
                        hash_table_set(headerHt, "Response Code", cval_string(header)); }
                }
                header = multi_tok(NULL, &x, "\r\n");
            }

            hash_table_set(resHt, "body", cval_string(strstr(s.body, "\r\n\r\n") + 4));
        } else {
            cval_delete(response_list);
#if SYSTEM_LANG == 0
            response_list = cval_fault("unable to accesh url!");
#else
            response_list = cval_fault("unable to access url!");
#endif
        }
        free(s.body);
    }
    hash_table_set(resHt, "headers", cval_dictionary(headerHt));
    curl_easy_cleanup(curl);
    cval * result = cval_dictionary(resHt);
    result->class = CVAL_CLASS_HTTP;
    return result;
}