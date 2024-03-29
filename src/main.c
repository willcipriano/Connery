#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "mpc.h"
#include "util.h"
#include "cval.h"
#include "hashtable.h"
#include "allocator.h"
#include "strings.h"
#include "http.h"
#include "json_parser.h"


#if TRACE_ENABLED == 1
#include "trace.h"
#else
typedef struct trace {} trace;
#endif

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

mpc_parser_t *Number;

mpc_parser_t *Float;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *DictionaryPair;
mpc_parser_t *Dictionary;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Connery;

#define CASSERT(args, cond, fmt, ...) \
if (!(cond)) {\
    cval* err = cval_fault(fmt, ##__VA_ARGS__); \
    cval_delete(args); \
    return err;}

#if SYSTEM_LANG == 0
#define CASSERT_TYPE(func, args, index, expect) \
  CASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' pashed incorrect type for argument %i. " \
    "Got %s, Expected %s.", \
    func, index, ctype_name(args->cell[index]->type), ctype_name(expect))
#else
#define CASSERT_TYPE(func, args, index, expect) \
  CASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. " \
    "Got %s, Expected %s.", \
    func, index, ctype_name(args->cell[index]->type), ctype_name(expect))
#endif

#if SYSTEM_LANG == 0
#define CASSERT_NUM(func, args, num) \
  CASSERT(args, args->count == num, \
    "function '%s' pashed incorrect number of argumentsh. got %i, expected %i.", \
    func, args->count, num)
#else
#define CASSERT_NUM(func, args, num) \
CASSERT(args, args->count == num, \
    "function '%s' passed incorrect number of arguments. got %i, expected %i.", \
    func, args->count, num)
#endif

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
#if SYSTEM_LANG == 0
                        x = cval_fault("Divishion by zero");
#else
                        x = cval_fault("Division by zero");
#endif
                        break;
                    }
                    x->fnum /= y->fnum;
                } else {
                    if (y->num == 0) {
                        cval_delete(x);
                        cval_delete(y);
#if SYSTEM_LANG == 0
                        x = cval_fault("Divishion by zero");
#else
                        x = cval_fault("Division by zero");
#endif
                        break;
                    }
                    x->fnum /= y->num;
                }
            }

            if (strcmp(op, "mod") == 0) {
                cval_delete(x);
                cval_delete(y);
#if SYSTEM_LANG == 0
                x = cval_fault("mod not shupported on floatsh!");
#else
                x = cval_fault("mod not supported on floats!");
#endif
                break;
            }

            if (strcmp(op, "pow") == 0) {
                cval_delete(x);
                cval_delete(y);
#if SYSTEM_LANG == 0
                x = cval_fault("pow not shupported on floatsh!");
#else
                x = cval_fault("pow not supported on floats!");
#endif
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
#if SYSTEM_LANG == 0
                    x = cval_fault("Divishion by zero");
#else
                    x = cval_fault("Division by zero");
#endif
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
#if SYSTEM_LANG == 0
        CASSERT(a, a->cell[0]->count != 0, "Function 'head' pashed empty list!");
#else
        CASSERT(a, a->cell[0]->count != 0, "Function 'head' passed empty list!");
#endif

        cval *v = cval_take(a, 0);

        while (v->count > 1) {
            cval_delete((cval_pop(v, 1)));
        }

        return v;
    }

    cval_delete(a);
#if SYSTEM_LANG == 0
    return cval_fault("Function 'head' pashed unshupported type!");
#else
    return cval_fault("Function 'head' passed unsupported type!");
#endif

}

#if TRACE_ENABLED == 1

cval *set_trace_data(cenv *e, trace *t) {
    hash_table_set(e->ht, "__STATEMENT_NUMBER__", cval_number(t->current->position));

    if (t->current->position > 2) {
        hash_table_set(e->ht, "__PREV_PREV_EXPRESSION__", hash_table_get(e->ht, "__PREV_EXPRESSION__"));
    }

    if (t->current->position > 1) {
        hash_table_set(e->ht, "__PREV_EXPRESSION__", t->current->prev->data);
    }

    hash_table_set(e->ht, "__EXPRESSION__", t->current->data);
}

#endif

cval *builtin_tail(cenv *e, cval *a) {
    CASSERT_NUM("tail", a, 1)

    if (a->cell[0]->type == CVAL_STRING) {
        char *str = cval_pop(a, 0)->str;
        cval_delete(a);

        if (strlen(str) > 0) {
            str++;
            return (cval_string(str));
        }
#if SYSTEM_LANG == 0
        return (cval_fault("Function 'tail' pashed empty shtring!"));
#else
        return (cval_fault("Function 'tail' passed empty string!"));
#endif
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
#if SYSTEM_LANG == 0
        CASSERT(a, a->cell[0]->count != 0, "Function 'tail' pashed empty list!");
#else
        CASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed empty list!");
#endif
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
#if SYSTEM_LANG == 0
            return cval_fault("Function 'tail' pashed shingle digit number!");
#else
            return cval_fault("Function 'tail' passed single digit number!");
#endif
        }

        factor = get_factor(init_digits);

        if (factor == -1) {
            return cval_fault("Unable to get factor for number!");
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
#if SYSTEM_LANG == 0
    return cval_fault("Function 'head' pashed unshupported type!");
#else
    return cval_fault("Function 'head' passed unsupported type!");
#endif
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

    if (result == 0) {
        return cval_boolean(false);
    }
    return cval_boolean(true);
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

        case CVAL_FAULT:
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

    if (r == 0) {
        return cval_boolean(false);
    }
    return cval_boolean(true);
}

cval *builtin_eq(cenv *e, cval *a) {
    return builtin_cmp(e, a, "==");
}

cval *builtin_ne(cenv *e, cval *a) {
    return builtin_cmp(e, a, "!=");
}

cval *builtin_if(cenv *e, cval *a) {
    CASSERT_NUM("if", a, 3)
    CASSERT_TYPE("if", a, 1, CVAL_Q_EXPRESSION)
    CASSERT_TYPE("if", a, 2, CVAL_Q_EXPRESSION)

    if (a->cell[0]->type != CVAL_NUMBER) {
        CASSERT_TYPE("if", a, 0, CVAL_BOOLEAN)
    }

    cval *x;
    a->cell[1]->type = CVAL_S_EXPRESSION;
    a->cell[2]->type = CVAL_S_EXPRESSION;

    if (a->cell[0]->type == CVAL_BOOLEAN) {
        if (a->cell[0]->boolean) {
            x = cval_evaluate(e, cval_pop(a, 1));
        } else {
            x = cval_evaluate(e, cval_pop(a, 2));
        }
    } else {
        if (a->cell[0]->num) {
            x = cval_evaluate(e, cval_pop(a, 1));
        } else {
            x = cval_evaluate(e, cval_pop(a, 2));
        }
    }

    cval_delete(a);
    return x;
}

cval *builtin_while(cenv *e, cval *a) {
    CASSERT_NUM("while", a, 2)
    CASSERT_TYPE("while", a, 0, CVAL_Q_EXPRESSION)
    CASSERT_TYPE("while", a, 1, CVAL_Q_EXPRESSION)

    cval *x;
    a->cell[0]->type = CVAL_S_EXPRESSION;
    a->cell[1]->type = CVAL_S_EXPRESSION;

    cval *condition_org = cval_pop(a, 0);
    cval *loop_org = cval_pop(a, 0);

    cval *condition = cval_copy(condition_org);
    cval *loop = cval_copy(loop_org);

    if (cval_evaluate(e, condition)->boolean) {
        bool condition_status = true;
        while (condition_status) {
            x = cval_evaluate(e, loop);

            condition = cval_copy(condition_org);
            condition_status = cval_evaluate(e, condition)->boolean;

            if (condition_status) {
                cval_delete(x);
                loop = cval_copy(loop_org);
            }
        }
    } else {
        return cval_s_expression();
    }

    cval_delete(loop_org);
    cval_delete(condition_org);
    cval_delete(a);
    return x;
}

cval *builtin_for(cenv *e, cval *a) {
    CASSERT_NUM("for", a, 2)
    CASSERT_TYPE("for", a, 0, CVAL_NUMBER)
    CASSERT_TYPE("for", a, 1, CVAL_Q_EXPRESSION)

    cval *x;
    int y = 1;
    a->cell[1]->type = CVAL_S_EXPRESSION;

    cval *loops = cval_pop(a, 0);
    cval *loop_org = cval_pop(a, 0);

    int q = loops->num;

    cval *loop = cval_copy(loop_org);

    while (y <= q) {
        x = cval_evaluate(e, loop);
        y += 1;
        if (y <= q) {
            cval_delete(x);
            loop = cval_copy(loop_org);

        }
    }

    cval_delete(loop_org);
    cval_delete(a);
    return x;
}

cval *cval_lambda(cval *formals, cval *body) {
    cval *v = allocate();
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

#if TRACE_ENABLED == 1
    trace *t = start_trace(a->cell[0]->str);
#endif

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Connery, &r)) {
        cval *expr = cval_read(r.output);
        mpc_ast_delete(r.output);


        while (expr->count) {
            cval *expression = cval_pop(expr, 0);
#if TRACE_ENABLED == 1
            record_trace(t, expression);
            set_trace_data(e, t);
#endif

            cval *x = cval_evaluate(e, expression);

            if (x->type == CVAL_FAULT) {
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

        cval *err = cval_fault("Could not load library '%s'", err_msg);
        free(err_msg);
        cval_delete(a);
        return err;
    }
}

cval *builtin_traced_load(cenv *e, cval *a, trace *t) {
    CASSERT_NUM("traced_load", a, 1)
    CASSERT_TYPE("traced_load", a, 0, CVAL_STRING)

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Connery, &r)) {
        cval *expr = cval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            cval *expression = cval_pop(expr, 0);
#if TRACE_ENABLED == 1
            record_trace(t, expression);
            set_trace_data(e, t);
#endif

            cval *x = cval_evaluate(e, expression);

            if (x->type == CVAL_FAULT) {
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

        cval *err = cval_fault("Could not load library '%s'", err_msg);
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

cval *builtin_fault(cenv *e, cval *a) {
    CASSERT_NUM("fault", a, 1);
    CASSERT_TYPE("fault", a, 0, CVAL_STRING);

    cval *err = cval_fault(a->cell[0]->str);

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
            return cval_fault("File not found!");
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
        return cval_fault("unable to write to file!");
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
        return cval_fault("unable to write to file!");
    }
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

    if (a->cell[0]->type == CVAL_DICTIONARY) {
        long length = a->cell[0]->ht->items;
        cval_delete(a);
        return cval_number(length);
    }

    cval_delete(a);
#if SYSTEM_LANG == 0
    return cval_fault("Function 'length' pashed unshupported type!");
#else
    return cval_fault("Function 'length' passed unsupported type!");
#endif

}

cval *builtin_type(cenv *e, cval *a) {
    int type = a->cell[0]->type;
    cval* returnVal = NULL;

    switch (type) {
        case CVAL_NUMBER:
            returnVal = cval_number(0);
            break;

        case CVAL_STRING:
            returnVal = cval_number(1);
            break;

        case CVAL_S_EXPRESSION:
            returnVal = cval_number(2);
            break;

        case CVAL_Q_EXPRESSION:
            returnVal = cval_number(3);
            break;

        case CVAL_FUNCTION:
            returnVal = cval_number(4);
            break;

        case CVAL_SYMBOL:
            returnVal = cval_number(5);
            break;

        case CVAL_BOOLEAN:
            returnVal = cval_number(6);
            break;

        case CVAL_FLOAT:
            returnVal = cval_number(7);
            break;

        case CVAL_DICTIONARY:
            returnVal = cval_number(8);
            break;

        case CVAL_NULL:
            returnVal = cval_number(9);
            break;

        default:
            returnVal = cval_fault("Type not defined!");
    }
    return returnVal;
}

cval *builtin_stow(cenv *e, cval *a) {
    CASSERT_TYPE("stow", a, 1, CVAL_STRING);

    if (a->count < 3) {
#if SYSTEM_LANG == 0
        return cval_fault("Stow requiresh at leasht three argumentsh."
                          "The dictionary, the key (ash a shtring of courshe) and the value to be shet.");
#else
        return cval_fault("Stow requires at least three aruguments."
                          "The dictionary, the key and the value to be set.");
#endif
    }
    cval* activeCval = NULL;

    if (a->cell[0]->type == CVAL_DICTIONARY) {
        activeCval = a->cell[0];
    } else {
        if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
            if (a->cell[0]->cell[0]->type == CVAL_DICTIONARY) {
                 activeCval = a->cell[0]->cell[0];
            }
        }
    }

    hash_table* ht = activeCval->ht;

    if (ht == NULL) {
        return cval_fault("Stow requiresh a dictionary, shtring for the key, and a value for the value.");
    }

    hash_table_set(ht, a->cell[1]->str, a->cell[2]);

    if (a->count >= 5) {
        int pos = 0;
        while (a->count > (pos + 3)) {

            if ((a->count - pos + 3) % 2 == 0) {
                hash_table_set(ht, a->cell[pos + 3]->str, a->cell[pos + 4]);
                pos += 2;
            } else {
                return cval_fault("Stow requiresh a even number of argumentsh, lad.");
            }
        }
    }
    return activeCval;
}

cval *builtin_grab(cenv *e, cval *a) {

    hash_table* ht = NULL;

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        if (a->cell[0]->cell[0]->type == CVAL_DICTIONARY) {
            ht = a->cell[0]->cell[0]->ht;
        }
    } else {
        if (a->cell[0]->type == CVAL_DICTIONARY) {
            ht = a->cell[0]->ht;
        }
    }

    if (ht == NULL) {
        return cval_fault("Grab requiresh at leasht two argumentsh, the dictionary to fetch from, and the key to fetch.");
    }

    if (a->count == 2) {
        cval *item = NULL;

        item = hash_table_get(ht, a->cell[1]->str);

        return item;
    }

    int idx = 1;
    cval *list = cval_q_expression();;
    while (idx < a->count) {
        if (a->cell[idx]->type == CVAL_STRING) {

        cval *item = NULL;

        item = hash_table_get(ht, a->cell[idx]->str);

        if (item == NULL) {
            cval_add(list, cval_null());
        } else {
            cval_add(list, cval_copy(item));
        }
        idx += 1;
    } else {
        cval_delete(list);
        cval_delete(a);
        return cval_fault("I can only grab itemsh via namesh defined in shtringsh, lad. Try again.");
    }
}

cval_delete(a);
return list;
}

cval *builtin_http(cenv *e, cval *a) {
    CASSERT_TYPE("http", a, 0, CVAL_STRING);
    return http_req_impl(e, a);
}

cval *builtin_json(cenv *e, cval *a) {
    return parse_json_cval(e, a->cell[0]);
}

cval *builtin_input(cenv *e, cval *a) {
    CASSERT_TYPE("input", a, 0, CVAL_STRING);
    char *input = readline(a->cell[0]->str);
    return cval_string(input);
}

cval *builtin_convert_string(cenv *e, cval *a) {
    cval* new;

    if (a->cell[0]->type == CVAL_NUMBER) {
        new = cval_string(int_to_string(a->cell[0]->num));
    }

    return new;
}

cval *builtin_inspect(cenv *e, cval *a) {
    CASSERT_NUM("inspect", a, 1);
    cval * value = a;
    cval* container_data = NULL;
    cval* env_data = NULL;
    cval* allocator_data = NULL;
    hash_table* ht = hash_table_create(15);
    hash_table* env_ht = hash_table_create(10);
    hash_table* allocator_ht = hash_table_create(2);

    if (a->type == CVAL_S_EXPRESSION) {
        if (a->count > 0) {
            hash_table* c_ht = hash_table_create(2);
            hash_table_set(c_ht, "container_id", cval_number(value->objId));
            container_data = cval_dictionary(c_ht);
            value = a->cell[0];
        }
    }

    if (container_data != NULL) {
        hash_table_set(ht, "container", container_data);
    }

    hash_table_set(ht, "id", cval_number(value->objId));
    hash_table_set(ht, "type", cval_string(ctype_name(value->type)));
    hash_table_set(ht, "count", cval_number(value->count));
    hash_table_set(ht, "is_deleted", cval_boolean(value->deleted));
    hash_table_set(ht, "is_marked", cval_boolean(value->mark));
    hash_table_set(ht, "has_children", cval_boolean(value->count > 0));

    hash_table_set(env_ht, "env_size", cval_number((long) e->ht->table_size));
    hash_table_set(env_ht, "env_items", cval_number((long) e->ht->items));

    env_data = cval_dictionary(env_ht);
    hash_table_set(ht, "env", env_data);

    long row = get_row_by_id(a->objId);
    hash_table_set(allocator_ht, "row", cval_number(row));
    hash_table_set(allocator_ht, "index", cval_number(get_index_by_row_and_id(a->objId, row)));
    allocator_data = cval_dictionary(ht);

    hash_table_set(ht, "allocator", allocator_data);

    if (value->type == CVAL_DICTIONARY) {
        hash_table_set(ht, "size", cval_number(value->ht->table_size));
        hash_table_set(ht, "items", cval_number(value->ht->items));
    }

    hash_table_set(ht, "hash", murmurhash_cval(value));

    return cval_dictionary(ht);
}

cval *builtin_sys(cenv *e, cval *a) {
    CASSERT_TYPE("stats", a, 0, CVAL_STRING);

    char *cmd = a->cell[0]->str;

    if (strcmp(cmd, "VERSION") == 0) {
        CASSERT_NUM("sys VERSION", a, 1);
        cval_delete(a);
        return cval_string(CONNERY_VERSION);
    }

    if (strcmp(cmd, "VERSION_INT") == 0) {
        CASSERT_NUM("sys VERSION_INT", a, 1);
        cval_delete(a);
        return cval_number(CONNERY_VER_INT);
    }

    if (strcmp(cmd, "PRINT_ENV") == 0) { ;
        CASSERT_NUM("sys PRINT_ENV", a, 1);
        cval_delete(a);
        return cval_number(hash_table_print(e->ht));
    }

    if (strcmp(cmd, "HARD_EXIT") == 0) {
        CASSERT_NUM("sys HARD_EXIT", a, 1);
        cval_delete(a);
        exit(1);
    }

    if (strcmp(cmd, "SOFT_EXIT") == 0) {
        CASSERT_NUM("sys SOFT_EXIT", a, 1);
        cval_delete(a);
        cenv_delete(e);
        index_shutdown();
        mpc_cleanup(8, Number, Float, Symbol, Sexpr, Qexpr, Expr, Comment, String, DictionaryPair, Dictionary, Connery);
        exit(0);
    }

    if (strcmp(cmd, "SYSTEM_LANGUAGE_INT") == 0) {
        CASSERT_NUM("sys SYSTEM_LANGUAGE_INT", a, 1);
        cval_delete(a);
        return cval_number(SYSTEM_LANG);
    }

    if (strcmp(cmd, "ALLOCATOR_STATUS") == 0) {
        CASSERT_NUM("sys ALLOCATOR_STATUS", a, 1);
        cval_delete(a);
        return allocator_status();
    }

    if (strcmp(cmd, "TAKE_OUT_THE_TRASH") == 0) {
        CASSERT_NUM("sys TAKE_OUT_THE_TRASH", a, 1);
        cval_delete(a);
        return mark_and_sweep(e);
    }

    if (strcmp(cmd, "BY_ID") == 0) {
        CASSERT_NUM("sys BY_ID", a, 2);
        cval* obj = object_by_id(a->cell[1]->objId);
        cval_delete(a);
        return obj;
    }

    return cval_fault("invalid input to stats");
}

cval* builtin_hash(cenv* e, cval* a) {
    cval *hash = murmurhash_cval(a->cell[0]);
    int cur = 1;

    while (cur < a->count) {
        hash = murmurhash_cval_with_seed(a->cell[cur], hash);
        cur += 1;
    }

    return hash;
}

void cenv_add_string_functions(cenv *e) {
    cenv_add_builtin(e, "string_replace_all", builtin_string_replace_all);
    cenv_add_builtin(e, "string_replace_first_char", builtin_string_replace_first_char);
    cenv_add_builtin(e, "string_concat", builtin_concat);
}


void cenv_add_builtins(cenv *e) {
    cenv_add_builtin(e, "\\", builtin_lambda);
    cenv_add_builtin(e, "def", builtin_def);
    cenv_add_builtin(e, "=", builtin_put);

    cenv_add_builtin(e, "stow", builtin_stow);
    cenv_add_builtin(e, "grab", builtin_grab);

    cenv_add_builtin(e, "list", builtin_list);
    cenv_add_builtin(e, "head", builtin_head);
    cenv_add_builtin(e, "tail", builtin_tail);
    cenv_add_builtin(e, "eval", builtin_eval);
    cenv_add_builtin(e, "join", builtin_join);
    cenv_add_builtin(e, "length", builtin_length);
    cenv_add_builtin(e, "input", builtin_input);
    cenv_add_builtin(e, "find", builtin_find);
    cenv_add_builtin(e, "split", builtin_split);
    cenv_add_builtin(e, "sys", builtin_sys);
    cenv_add_builtin(e, "inspect", builtin_inspect);
    cenv_add_builtin(e, "hash", builtin_hash);

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
    cenv_add_builtin(e, "print", builtin_print);
    cenv_add_builtin(e, "type", builtin_type);
    cenv_add_builtin(e, "http", builtin_http);
    cenv_add_builtin(e, "json", builtin_json);

    cenv_add_builtin(e, "file", builtin_file);
    cenv_add_builtin(e, "convert_string", builtin_convert_string);

    cenv_add_string_functions(e);

    cenv_add_builtin(e, "while", builtin_while);
    cenv_add_builtin(e, "for", builtin_for);

    cenv_add_builtin(e, "__FAULT__", builtin_fault);
}

void load_standard_lib(cenv *e) {
    cval *stdLib = cval_add(cval_s_expression(), cval_string("stdlib/main.connery"));
    if (stdLib->type == CVAL_FAULT) {
        cval_print_line(stdLib);
    }
    builtin_load(e, stdLib);
}

int main(int argc, char **argv) {
    allocator_setup();

    Number = mpc_new("number");
    Float = mpc_new("float");
    Symbol = mpc_new("symbol");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Comment = mpc_new("comment");
    String = mpc_new("string");
    DictionaryPair = mpc_new("dictionary_pair");
    Dictionary = mpc_new("dictionary");
    Connery = mpc_new("connery");

    mpca_lang(MPCA_LANG_DEFAULT,
              "                                               \
                float     : /-?[0-9]*\\.[0-9]+/ ;                     \
                number    : /-?[0-9]+/ ;                              \
                symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/          \
                            |'+' | '-' | '*' | '/' ;                  \
                sexpr     : '(' <expr>* ')' ;                         \
                qexpr     : '{' <expr>* '}' ;                         \
                dictionary_pair : <string> '&' <expr> /[,]?/ ;        \
                dictionary : '#' <dictionary_pair>* '#' ;             \
                string    : /\"(\\\\.|[^\"])*\"/;                     \
                comment : /;[^\\r\\n]*/  ;                            \
                expr      : <float>  | <number> | <symbol>            \
                          | <comment> | <sexpr> | <qexpr> | <string>  \
                          | <dictionary>  ;                           \
                connery   : /^/ <expr>* /$/ ;                         \
            ",
              Float, Number, Symbol, Sexpr, Qexpr, DictionaryPair, Dictionary, Expr, String, Comment, Connery);

    cenv *e = cenv_new();

    hash_table_set(e->ht, "Null", cval_null());
    hash_table_set(e->ht, "True", cval_boolean(true));
    hash_table_set(e->ht, "False", cval_boolean(false));
    hash_table_set(e->ht, "Empty", cval_string(""));
    hash_table_set(e->ht, "None", cval_s_expression());
    hash_table_set(e->ht, "Otherwise", cval_boolean(true));
    hash_table_set(e->ht, "__std_lib_main_location__", cval_string(STD_LIB_LOCATION));
    hash_table_set(e->ht, "__BK_13__", cval_string("Nyvpr Vzbtra Pvcevnab"));

    cenv_add_builtins(e);
    load_standard_lib(e);
    mark_and_sweep(e);

    puts("   ______                                 \n"
         "  / ____/___  ____  ____  ___  _______  __\n"
         " / /   / __ \\/ __ \\/ __ \\/ _ \\=/ ___/ / / /\n"
         "/ /___/ /_/ / / / / / / /  __/ /  / /_/ / \n"
         "\\____/\\____/_/ /_/_/ /_/\\___/_/   \\__, /  \n"
         "                                 /____/   ");
#if SYSTEM_LANG == 1
    puts("_____________ English Mode _____________");
#endif
    puts("            Version "CONNERY_VERSION);
    puts("           ConneryLang.org             \n");

    hash_table_set(e->ht, "__LOG_LEVEL__", cval_number(LOG_LEVEL));


    if (argc == 1) {
        hash_table_set(e->ht, "__SOURCE__", cval_string("INTERACTIVE"));
#if TRACE_ENABLED == 1
        trace *trace = start_trace("interactive");
#endif
        while (1) {
            char *input = readline("connery> ");
            add_history(input);

#if TRACE_ENABLED == 1
            record_trace(trace, cval_string(input));
            set_trace_data(e, trace);
#endif
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
            allocator_check(e);
        }
    }

    if (argc >= 2) {
        hash_table_set(e->ht, "__SOURCE__", cval_string("FILE"));
#if TRACE_ENABLED == 1
        trace *trace = start_trace("FILE");
#endif
        for (int i = 1; i < argc; i++) {
            cval *args = cval_add(cval_s_expression(), cval_string(argv[i]));
            hash_table_set(e->ht, "__SOURCE_FILE__", cval_string(argv[i]));
#if TRACE_ENABLED == 1
            cval *x = builtin_traced_load(e, args, trace);
#else
            cval *x = builtin_load(e, args);
#endif
            if (x->type == CVAL_FAULT) {
                cval_print_line(x);
            }

            cval_delete(x);
            allocator_check(e);
        }
    }

    mpc_cleanup(8, Number, Float, Symbol, Sexpr, Qexpr, Expr, Comment, String, DictionaryPair, Dictionary, Connery);
    index_shutdown();
    cenv_delete(e);
    exit(0);
}
