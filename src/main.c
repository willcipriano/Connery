#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include "mpc.h"
#include "util.h"
#include "cval.h"
#include "hashtable.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}
#else

#include <editline/history.h>

#endif
mpc_parser_t *Number;
mpc_parser_t *Float;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Connery;

#define CASSERT(args, cond, fmt, ...) \
if (!(cond)) {\
    cval* err = cval_error(fmt, ##__VA_ARGS__); \
    cval_delete(args); \
    return err;}

#define CASSERT_TYPE(func, args, index, expect) \
  CASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' pashed incorrect type for argument %i. " \
    "Got %s, Expected %s.", \
    func, index, ctype_name(args->cell[index]->type), ctype_name(expect))

#define CASSERT_NUM(func, args, num) \
  CASSERT(args, args->count == num, \
    "function '%s' pashed incorrect number of argumentsh. got %i, expected %i.", \
    func, args->count, num)

cval *builtin_op(cenv *e, cval *a, char *op) {
    int float_support = 0;

    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type == CVAL_FLOAT) {
            float_support = 1;
        } else { CASSERT_TYPE(op, a, i, CVAL_NUMBER); }
    }

    if (float_support) {
        cval *x;
        if (a->cell[0]->type == CVAL_FLOAT) {
            x = cval_pop(a, 0);
        } else {
            x = cval_float(cval_pop(a, 0)->num);
        }

        if ((strcmp(op, "-") == 0) && a->count == 0) {
            x->fnum = -x->fnum;
        }

        while (a->count > 0) {
            cval *y = cval_pop(a, 0);

            if (strcmp(op, "+") == 0) {
                if (y->type == CVAL_FLOAT) {
                    x->fnum += y->fnum;
                } else {
                    x->fnum += y->num;
                }
            }

            if (strcmp(op, "-") == 0) {
                if (y->type == CVAL_FLOAT) {
                    x->fnum -= y->fnum;
                } else {
                    x->fnum -= y->num;
                }
            }

            if (strcmp(op, "*") == 0) {
                if (y->type == CVAL_FLOAT) {
                    x->fnum *= y->fnum;
                } else {
                    x->fnum *= y->num;
                }
            }

            if (strcmp(op, "/") == 0) {
                if (y->type == CVAL_FLOAT) {
                    if (y->fnum == 0) {
                        cval_delete(x);
                        cval_delete(y);
                        x = cval_error("Divishion by zero");
                        break;
                    }
                    x->fnum /= y->fnum;
                } else {
                    if (y->num == 0) {
                        cval_delete(x);
                        cval_delete(y);
                        x = cval_error("Divishion by zero");
                        break;
                    }
                    x->fnum /= y->num;
                }
            }

            if (strcmp(op, "mod") == 0) {
                cval_delete(x);
                cval_delete(y);
                x = cval_error("mod not shupported on floatsh!");
                break;
            }

            if (strcmp(op, "pow") == 0) {
                cval_delete(x);
                cval_delete(y);
                x = cval_error("pow not shupported on floatsh!");
                break;
            }

            cval_delete(y);
        }
        cval_delete(a);
        return x;
    } else {

        cval *x = cval_pop(a, 0);

        if ((strcmp(op, "-") == 0) && a->count == 0) {
            x->num = -x->num;
        }

        while (a->count > 0) {
            cval *y = cval_pop(a, 0);

            if (strcmp(op, "+") == 0) {
                x->num += y->num;
            }

            if (strcmp(op, "-") == 0) {
                x->num -= y->num;
            }

            if (strcmp(op, "*") == 0) {
                x->num *= y->num;
            }

            if (strcmp(op, "/") == 0) {
                if (y->num == 0) {
                    cval_delete(x);
                    cval_delete(y);
                    x = cval_error("Divishion by zero");
                    break;
                }
                x->num /= y->num;
            }

            if (strcmp(op, "mod") == 0) {
                x->num = x->num % y->num;
            }

            if (strcmp(op, "pow") == 0) {
                x->num = long_power(x->num, y->num);
            }

            cval_delete(y);
        }
        cval_delete(a);
        return x;
    }
}

cval *builtin_head(cenv *e, cval *a) {
    CASSERT_NUM("head", a, 1)

    if (a->cell[0]->type == CVAL_STRING) {
        char *first_char;
        first_char = malloc(2);
        first_char[0] = cval_pop(a, 0)->str[0];
        first_char[1] = '\0';
        cval_delete(a);
        cval *result = cval_string(first_char);
        return result;
    }

    if (a->cell[0]->type == CVAL_NUMBER) {
        long n = cval_pop(a, 0)->num;
        long first_digit;

        while (n) {
            first_digit = n;
            n /= 10;
        }
        cval_delete(a);
        return cval_number(first_digit);
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        CASSERT(a, a->cell[0]->count != 0, "Function 'head' pashed empty list!");

        cval *v = cval_take(a, 0);

        while (v->count > 1) {
            cval_delete((cval_pop(v, 1)));
        }

        return v;
    }

    cval_delete(a);
    return cval_error("Function 'head' pashed unshupported type!");
}

cval *builtin_tail(cenv *e, cval *a) {
    CASSERT_NUM("tail", a, 1)

    if (a->cell[0]->type == CVAL_STRING) {
        char *str = cval_pop(a, 0)->str;
        cval_delete(a);

        if (strlen(str) > 0) {
            str++;
            return (cval_string(str));
        }
        return (cval_error("Function 'tail' pashed empty shtring!"));
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        CASSERT(a, a->cell[0]->count != 0, "Function 'tail' pashed empty list!");

        cval *v = cval_take(a, 0);

        cval_delete(cval_pop(v, 0));
        return v;
    }

    if (a->cell[0]->type == CVAL_NUMBER) {
        long number = cval_pop(a, 0)->num;
        int neg = 0;

        if (number < 0) {
            neg = 1;
            number = labs(number);
        }

        int init_digits = count_digits(number);
        long factor;
        cval_delete(a);

        if (init_digits == 1) {
            return cval_error("Function 'tail' pashed shingle digit number!");
        }

        factor = get_factor(init_digits);

        if (factor == -1) {
            return cval_error("Unable to get factor for number!");
        }

        while (count_digits(number) == init_digits) {
            number -= factor;
        }

        if (neg) {
            number = -labs(number);
        }
        return cval_number(number);
    }

    cval_delete(a);
    return cval_error("Function 'head' pashed unshupported type!");
}

cval *builtin_rand(cenv *e, cval *a) {
    CASSERT_NUM("rand", a, 2)
    CASSERT_TYPE("rand", a, 0, CVAL_NUMBER);
    CASSERT_TYPE("rand", a, 1, CVAL_NUMBER);

    long rand_start = a->cell[0]->num;
    long rand_end = a->cell[1]->num;
    cval_delete(a);

    long rand_result = (rand() %
                        (rand_end - rand_start + 1)) + rand_start;

    return cval_number(rand_result);
}

cval *builtin_join(cenv *e, cval *a) {


    if (a->cell[0]->type == CVAL_STRING) {
        unsigned long new_str_length = 0;

        for (int i = 0; i < a->count; i++) {
            CASSERT_TYPE("join", a, i, CVAL_STRING)
            new_str_length += strlen(a->cell[i]->str);
        }

        char *new_str;
        new_str = malloc(new_str_length + 1);
        new_str[0] = '\0';

        for (int i = 0; i < a->count; i++) {
            new_str = strcat(new_str, a->cell[i]->str);
        }

        cval_delete(a);
        cval *result = cval_string(new_str);
        free(new_str);
        return result;
    }

    for (int i = 0; i < a->count; i++) {
        CASSERT_TYPE("join", a, i, CVAL_Q_EXPRESSION)
    }

    cval *x = cval_pop(a, 0);

    while (a->count) {
        x = cval_join(x, cval_pop(a, 0));
    }

    cval_delete(a);

    return x;
}

cval *builtin_add(cenv *e, cval *a) {
    return builtin_op(e, a, "+");
}

cval *builtin_sub(cenv *e, cval *a) {
    return builtin_op(e, a, "-");
}

cval *builtin_mul(cenv *e, cval *a) {
    return builtin_op(e, a, "*");
}

cval *builtin_div(cenv *e, cval *a) {
    return builtin_op(e, a, "/");
}

cval *builtin_modulo(cenv *e, cval *a) {
    return builtin_op(e, a, "mod");
}

cval *builtin_power(cenv *e, cval *a) {
    return builtin_op(e, a, "pow");
}

cval *builtin_order(cenv *e, cval *a, char *op) {
    CASSERT_NUM(op, a, 2);
    CASSERT_TYPE(op, a, 0, CVAL_NUMBER);
    CASSERT_TYPE(op, a, 1, CVAL_NUMBER);

    int result;

    if (strcmp(op, ">") == 0) {
        result = (a->cell[0]->num > a->cell[1]->num);
    }

    if (strcmp(op, "<") == 0) {
        result = (a->cell[0]->num < a->cell[1]->num);
    }

    if (strcmp(op, ">=") == 0) {
        result = (a->cell[0]->num >= a->cell[1]->num);
    }

    if (strcmp(op, "<=") == 0) {
        result = (a->cell[0]->num <= a->cell[1]->num);
    }
    cval_delete(a);
    return cval_number(result);
}

cval *builtin_greater_than_or_equal(cenv *e, cval *a) {
    return builtin_order(e, a, ">=");
}

cval *builtin_less_than_or_equal(cenv *e, cval *a) {
    return builtin_order(e, a, "<=");
}

cval *builtin_greater_than(cenv *e, cval *a) {
    return builtin_order(e, a, ">");
}

cval *builtin_less_than(cenv *e, cval *a) {
    return builtin_order(e, a, "<");
}

int cval_equal(cval *x, cval *y) {

    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case CVAL_NUMBER:
            return (x->num == y->num);

        case CVAL_ERROR:
        case CVAL_SYMBOL:
            return (strcmp(x->err, y->err) == 0);

        case CVAL_FUNCTION:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return cval_equal(x->formals, y->formals) && cval_equal(x->body, y->body);
            }

        case CVAL_Q_EXPRESSION:
        case CVAL_S_EXPRESSION:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                if (!cval_equal(x->cell[i], y->cell[i])) { return 0; }
            }
            return 1;

        case CVAL_STRING:
            return (strcmp(x->str, y->str) == 0);
    }
    return 0;
}

cval *builtin_cmp(cenv *e, cval *a, char *op) {
    CASSERT_NUM(op, a, 2);
    int r;

    if (strcmp(op, "==") == 0) {
        r = cval_equal(a->cell[0], a->cell[1]);
    }

    if (strcmp(op, "!=") == 0) {
        r = !cval_equal(a->cell[0], a->cell[1]);
    }

    cval_delete(a);
    return cval_number(r);
}

cval *builtin_eq(cenv *e, cval *a) {
    return builtin_cmp(e, a, "==");
}

cval *builtin_ne(cenv *e, cval *a) {
    return builtin_cmp(e, a, "!=");
}

cval *builtin_if(cenv *e, cval *a) {
    CASSERT_NUM("if", a, 3)
    CASSERT_TYPE("if", a, 0, CVAL_NUMBER)
    CASSERT_TYPE("if", a, 1, CVAL_Q_EXPRESSION)
    CASSERT_TYPE("if", a, 2, CVAL_Q_EXPRESSION)

    cval *x;
    a->cell[1]->type = CVAL_S_EXPRESSION;
    a->cell[2]->type = CVAL_S_EXPRESSION;

    if (a->cell[0]->num) {
        x = cval_evaluate(e, cval_pop(a, 1));
    } else {
        x = cval_evaluate(e, cval_pop(a, 2));
    }

    cval_delete(a);
    return x;
}

cval *cval_lambda(cval *formals, cval *body) {
    cval *v = malloc(sizeof(cval));
    v->type = CVAL_FUNCTION;

    v->builtin = NULL;

    v->env = cenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

cval *builtin_lambda(cenv *e, cval *a) {
    CASSERT_NUM("lambda", a, 2)
    CASSERT_TYPE("\\", a, 0, CVAL_Q_EXPRESSION);
    CASSERT_TYPE("\\", a, 1, CVAL_Q_EXPRESSION);

    for (int i = 0; i < a->cell[0]->count; i++) {
        CASSERT(a, (a->cell[0]->cell[i]->type == CVAL_SYMBOL), "Cannot define non-symbol. Got %s, Expected %s.",
                ctype_name(a->cell[0]->cell[i]->type), ctype_name(CVAL_SYMBOL));
    }

    cval *formals = cval_pop(a, 0);
    cval *body = cval_pop(a, 0);
    cval_delete(a);

    return cval_lambda(formals, body);
}

cval *builtin_load(cenv *e, cval *a) {
    CASSERT_NUM("load", a, 1)
    CASSERT_TYPE("load", a, 0, CVAL_STRING)

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Connery, &r)) {
        cval *expr = cval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            cval *x = cval_evaluate(e, cval_pop(expr, 0));

            if (x->type == CVAL_ERROR) {
                cval_print_line(x);
            }
            cval_delete(x);
        }

        cval_delete(expr);
        cval_delete(a);

        return cval_s_expression();
    } else {
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        cval *err = cval_error("Could not load library '%s'", err_msg);
        free(err_msg);
        cval_delete(a);
        return err;
    }
}

cval *builtin_print(cenv *e, cval *a) {
    for (int i = 0; i < a->count; i++) {
        cval_print(a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    cval_delete(a);

    return cval_s_expression();
}

cval *builtin_error(cenv *e, cval *a) {
    CASSERT_NUM("error", a, 1);
    CASSERT_TYPE("error", a, 0, CVAL_STRING);

    cval *err = cval_error(a->cell[0]->str);

    cval_delete(a);
    return err;
}

cval *builtin_file(cenv *e, cval *a) {
    CASSERT_TYPE("file", a, 0, CVAL_STRING);
    CASSERT_TYPE("file", a, 1, CVAL_STRING);

    char *operation = cval_pop(a, 0)->str;
    char *fileLocation = cval_pop(a, 0)->str;
    FILE *file;

    if (strstr(operation, "READ")) {
        char *buffer = 0;
        long length;

        file = fopen(fileLocation, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            length = ftell(file);
            fseek(file, 0, SEEK_SET);
            buffer = malloc(length);

            if (buffer) {
                fread(buffer, 1, length, file);
            }
            fclose(file);
        }

        if (buffer) {
            cval_delete(a);
            return cval_string(buffer);
        } else {
            cval_delete(a);
            return cval_error("File not found!");
        }
    }

    if (strstr(operation, "SAVE")) {
        CASSERT_TYPE("file", a, 0, CVAL_STRING);
        char *file_string = cval_pop(a, 0)->str;

        file = fopen(fileLocation, "w+");

        if (file) {
            int success = fputs(file_string, file);
            if (success) {
                fclose(file);
                cval_delete(a);
                return cval_number(1);
            }
        }
        cval_delete(a);
        return cval_error("unable to write to file!");
    }

    if (strstr(operation, "APPEND")) {
        CASSERT_TYPE("file", a, 0, CVAL_STRING);
        char *file_string = cval_pop(a, 0)->str;

        file = fopen(fileLocation, "a");

        if (file) {
            int success = fputs(file_string, file);
            if (success) {
                fclose(file);
                cval_delete(a);
                return cval_number(1);
            }
        }
        cval_delete(a);
        return cval_error("unable to write to file!");
    }
}

cval *builtin_replace(cenv *e, cval *a) {
    CASSERT_TYPE("replace", a, 0, CVAL_STRING);
    CASSERT_TYPE("replace", a, 1, CVAL_STRING);
    CASSERT_TYPE("replace", a, 2, CVAL_STRING);
    CASSERT_NUM("replace", a, 3);

    char *orig = a->cell[0]->str;
    char *rep = a->cell[1]->str;
    char *with = a->cell[2]->str;

    cval *result;

    if (strstr(orig, rep)) {
        multi_tok_t y = multiTok_init();
        char *start_string = multi_tok(orig, &y, rep);
        char *new_string = concatenateThree(start_string, with, y);

        result = cval_string(new_string);
    } else {
        result = cval_string(orig);
    }

    cval_delete(a);

    return result;
}

cval *builtin_find(cenv *e, cval *a) {
    CASSERT_TYPE("find", a, 0, CVAL_STRING);
    CASSERT_TYPE("find", a, 1, CVAL_STRING);
    CASSERT_NUM("find", a, 2);

    char *pos;
    char *org = cval_pop(a, 0)->str;

    if ((pos = strstr(org, cval_pop(a, 0)->str))) {
        cval_delete(a);
        return cval_number(pos - org);
    } else {
        cval_delete(a);
        return cval_number(0);
    }
}

cval *builtin_split(cenv *e, cval *a) {
    CASSERT_TYPE("split", a, 0, CVAL_STRING);
    CASSERT_TYPE("split", a, 1, CVAL_NUMBER);
    CASSERT_NUM("split", a, 2);

    char *original_string = cval_pop(a, 0)->str;
    long split_index = cval_pop(a, 0)->num;
    long str_length = strlen(original_string);

    if (str_length <= split_index) {
        cval_delete(a);
        return cval_error("Index %i out of bounds", split_index);
    }

    char first_half[split_index];
    char second_half[str_length - split_index];

    for (int i = 0; i <= str_length; ++i) {
        if (i < split_index) {
            first_half[i] = original_string[i];
            first_half[i + 1] = '\0';
        } else {
            second_half[i - split_index] = original_string[i];
        }
    }

    cval *list = cval_q_expression();
    cval *first = cval_string(first_half);
    cval *second = cval_string(second_half);

    cval_add(list, first);
    cval_add(list, second);

    cval_delete(a);

    return list;
}

cval *builtin_length(cenv *e, cval *a) {
    CASSERT_NUM("length", a, 1);

    if (a->cell[0]->type == CVAL_NUMBER) {
        long length = count_digits(a->cell[0]->num);
        cval_delete(a);
        return cval_number(length);
    }

    if (a->cell[0]->type == CVAL_STRING) {
        long length = strlen(cval_pop(a, 0)->str);
        cval_delete(a);
        return cval_number(length);
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        long length = a->cell[0]->count;
        cval_delete(a);
        return cval_number(length);
    }

    cval_delete(a);
    return cval_error("Function 'length' pashed unshupported type!");
}

cval *builtin_type(cenv *e, cval *a) {
    int type = a->cell[0]->type;
    cval_delete(a);

    switch (type) {
        case CVAL_NUMBER:
            return cval_number(0);

        case CVAL_STRING:
            return cval_number(1);

        case CVAL_S_EXPRESSION:
            return cval_number(2);

        case CVAL_Q_EXPRESSION:
            return cval_number(3);

        case CVAL_FUNCTION:
            return cval_number(4);

        case CVAL_SYMBOL:
            return cval_number(5);

        default:
            return cval_error("Type not defined!");
    }
}

cval *builtin_http(cenv *e, cval *a) {
    CASSERT_TYPE("http", a, 0, CVAL_STRING);
    CASSERT_TYPE("http", a, 1, CVAL_STRING);
    CASSERT_TYPE("http", a, 2, CVAL_Q_EXPRESSION);


    CURL *curl;
    CURLcode res;
    long response_code;
    cval *response_list = cval_q_expression();
    cval *headers_list = cval_q_expression();
    char *html_body;

    char *type = a->cell[0]->str;
    char *url = a->cell[1]->str;
    cval *request_headers = a->cell[2];

    curl = curl_easy_init();
    if (curl) {
        struct http_response s;
        init_http_response(&s);
        struct curl_slist *chunk = NULL;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_response_writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Connery");
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

        for (int i = 0; i < request_headers->count; i++) {
            CASSERT_TYPE("http_request_headers", request_headers, i, CVAL_STRING);
            chunk = curl_slist_append(chunk, request_headers->cell[i]->str);
        }

        if (strstr(type, "POST")) {
            CASSERT_TYPE("http", a, 3, CVAL_STRING);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, a->cell[3]->str);
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            cval_add(response_list, cval_number(response_code));

            char *response;
            response = malloc(strlen(s.body) + 1);
            strcpy(response, s.body);

            multi_tok_t y = multiTok_init();
            char *headers = multi_tok(response, &y, "\r\n\r\n");

            multi_tok_t x = multiTok_init();
            char *header = multi_tok(headers, &x, "\r\n");

            while (header != NULL) {
                cval_add(headers_list, cval_string(header));
                header = multi_tok(NULL, &x, "\r\n");
            }

            cval_add(response_list, headers_list);
            cval_add(response_list, cval_string(strstr(s.body, "\r\n\r\n") + 4));
        } else {
            cval_delete(response_list);
            response_list = cval_error("unable to accesh url!");
        }
        free(s.body);
        curl_easy_cleanup(curl);
    }
    return response_list;
}

cval *builtin_input(cenv *e, cval *a) {
    CASSERT_TYPE("input", a, 0, CVAL_STRING);
    char *input = readline(a->cell[0]->str);
    return cval_string(input);
}


void cenv_add_builtins(cenv *e) {
    cenv_add_builtin(e, "\\", builtin_lambda);
    cenv_add_builtin(e, "def", builtin_def);
    cenv_add_builtin(e, "=", builtin_put);

    cenv_add_builtin(e, "list", builtin_list);
    cenv_add_builtin(e, "head", builtin_head);
    cenv_add_builtin(e, "tail", builtin_tail);
    cenv_add_builtin(e, "eval", builtin_eval);
    cenv_add_builtin(e, "join", builtin_join);
    cenv_add_builtin(e, "def", builtin_def);
    cenv_add_builtin(e, "length", builtin_length);
    cenv_add_builtin(e, "input", builtin_input);
    cenv_add_builtin(e, "replace", builtin_replace);
    cenv_add_builtin(e, "find", builtin_find);
    cenv_add_builtin(e, "split", builtin_split);

    cenv_add_builtin(e, "+", builtin_add);
    cenv_add_builtin(e, "-", builtin_sub);
    cenv_add_builtin(e, "*", builtin_mul);
    cenv_add_builtin(e, "/", builtin_div);
    cenv_add_builtin(e, "mod", builtin_modulo);
    cenv_add_builtin(e, "pow", builtin_power);
    cenv_add_builtin(e, "rand", builtin_rand);

    cenv_add_builtin(e, "if", builtin_if);
    cenv_add_builtin(e, "==", builtin_eq);
    cenv_add_builtin(e, "!=", builtin_ne);
    cenv_add_builtin(e, ">", builtin_greater_than);
    cenv_add_builtin(e, "<", builtin_less_than);
    cenv_add_builtin(e, ">=", builtin_greater_than_or_equal);
    cenv_add_builtin(e, "<=", builtin_less_than_or_equal);

    cenv_add_builtin(e, "load", builtin_load);
    cenv_add_builtin(e, "error", builtin_error);
    cenv_add_builtin(e, "print", builtin_print);
    cenv_add_builtin(e, "type", builtin_type);
    cenv_add_builtin(e, "http", builtin_http);

    cenv_add_builtin(e, "file", builtin_file);
}

void load_standard_lib(cenv *e) {
    cval *stdLib = cval_add(cval_s_expression(), cval_string("stdlib/main.connery"));
    if (stdLib->type == CVAL_ERROR) {
        cval_print_line(stdLib);
    }
    builtin_load(e, stdLib);
}

int main(int argc, char **argv) {
    Number = mpc_new("number");
    Float = mpc_new("float");
    Symbol = mpc_new("symbol");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Comment = mpc_new("comment");
    String = mpc_new("string");
    Connery = mpc_new("connery");

    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                 \
                float     : /-?[0-9]*\\.[0-9]+/ ;                     \
                number    : /-?[0-9]+/ ;                              \
                symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/          \
                            |'+' | '-' | '*' | '/' ;                  \
                sexpr     : '(' <expr>* ')' ;                         \
                qexpr     : '{' <expr>* '}' ;                         \
                string    : /\"(\\\\.|[^\"])*\"/;                     \
                comment : /;[^\\r\\n]*/  ;                            \
                expr      : <float>  | <number> | <symbol>             \
                          | <comment> | <sexpr> | <qexpr> | <string> ;\
                connery   : /^/ <expr>* /$/ ;                         \
            ",
              Float, Number, Symbol, Sexpr, Qexpr, Expr, String, Comment, Connery);

    cenv *e = cenv_new();
    cenv_add_builtins(e);
    load_standard_lib(e);

    puts("   ______                                 \n"
         "  / ____/___  ____  ____  ___  _______  __\n"
         " / /   / __ \\/ __ \\/ __ \\/ _ \\/ ___/ / / /\n"
         "/ /___/ /_/ / / / / / / /  __/ /  / /_/ / \n"
         "\\____/\\____/_/ /_/_/ /_/\\___/_/   \\__, /  \n"
         "                                 /____/   ");
    puts("+-----      Version 0.0.4      ------+\n");
    puts("           ConneryLang.org             \n");

    if (argc == 1) {
        while (1) {
            char *input = readline("connery> ");
            add_history(input);

            mpc_result_t result;
            if (mpc_parse("<stdin>", input, Connery, &result)) {
                cval *output = cval_evaluate(e, cval_read(result.output));
                cval_print_line(output);
                cval_delete(output);

                mpc_ast_delete(result.output);
            } else {
                mpc_err_print(result.error);
                mpc_err_delete(result.error);
            }
            free(input);
        }
    }

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            cval *args = cval_add(cval_s_expression(), cval_string(argv[i]));
            cval *x = builtin_load(e, args);

            if (x->type == CVAL_ERROR) {
                cval_print_line(x);
            }
            cval_delete(x);
        }
    }

    mpc_cleanup(8, Number, Float, Symbol, String, Comment, Sexpr, Qexpr, Expr, Connery);
    cenv_delete(e);
    return 0;
}