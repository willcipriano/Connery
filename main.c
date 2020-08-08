#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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

struct cval;
struct cenv;
typedef struct cval cval;
typedef struct cenv cenv;

enum {CVAL_NUMBER, CVAL_ERROR, CVAL_SYMBOL, CVAL_FUNCTION,
        CVAL_S_EXPRESSION, CVAL_Q_EXPRESSION, CVAL_STRING};

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Connery;

typedef cval*(*cbuiltin)(cenv*, cval*);
void cenv_delete(cenv* e);
void cval_print(cval* value);
cval* cval_pop(cval* value, int i);
cval* cval_evaluate(cenv* env,cval* value);
cval* cval_take(cval* value, int i);

char* ctype_name(int t) {
    switch(t) {
        case CVAL_FUNCTION: return "Function";
        case CVAL_NUMBER: return "Number";
        case CVAL_ERROR: return "Error";
        case CVAL_SYMBOL: return "Symbol";
        case CVAL_S_EXPRESSION: return "S-Expression";
        case CVAL_Q_EXPRESSION: return "Q-Expression";
        case CVAL_STRING: return "String";
        default: return "Unknown Type";
    }
}

struct cval {
    int type;

    long num;
    char* err;
    char* sym;
    char* str;

    cbuiltin builtin;
    cenv* env;
    cval* formals;
    cval* body;

    int count;
    cval** cell;
};

struct cenv {
    cenv* par;
    int count;
    char** symbols;
    cval** values;
};

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

#define CASSERT_TYPE_ITER(func, args, index, iter_index, expect) \
  CASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' pashed incorrect type for argument %i. " \
    "Got %s, Expected %s.", \
    func, iter_index, ctype_name(args->cell[index]->type), ctype_name(expect))

#define CASSERT_NUM(func, args, num) \
  CASSERT(args, args->count == num, \
    "function '%s' pashed incorrect number of argumentsh. got %i, expected %i.", \
    func, args->count, num)

int count_digits(long n)
{
    if (n == 0)
        return 0;
    return 1 + count_digits(n / 10);
}

long long_power(long x,long exponent)
{
    int i;
    int number = 1;

    for (i = 0; i < exponent; ++i)
        number *= x;

    return(number);
}


cval* cval_function(cbuiltin func) {
    cval* v = malloc(sizeof(cval));
    v->type = CVAL_FUNCTION;
    v->builtin = func;
    return v;
}

cval* cval_number(long x) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_NUMBER;
    value->num = x;
    return value;
}

cval* cval_string (char* s) {
    cval* v = malloc(sizeof(cval));
    v->type = CVAL_STRING;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

cval* cval_error(char* fmt, ...) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_ERROR;

    va_list va;
    va_start(va, fmt);

    value->err = malloc(512);

    vsnprintf(value->err, 511, fmt, va);

    value->err = realloc(value->err, strlen(value->err)+1);

    va_end(va);

    return value;
}

cval* cval_symbol(char* s) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_SYMBOL;
    value->sym = malloc(strlen(s) + 1);
    strcpy(value->sym, s);
    return value;
}

cval* cval_s_expression(void) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_S_EXPRESSION;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cval* cval_q_expression(void) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_Q_EXPRESSION;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cenv* cenv_new(void) {
    cenv* e = malloc(sizeof(cenv));
    e->par = NULL;
    e->count = 0;
    e->symbols = NULL;
    e->values = NULL;
    return e;
}

void cval_delete(cval* value) {
    switch(value->type) {

        case CVAL_NUMBER:
            break;

        case CVAL_FUNCTION:
            if (!value->builtin) {
                cenv_delete(value->env);
                cval_delete(value->formals);
                cval_delete(value->body);
            }
            break;

        case CVAL_ERROR:
            free(value->err);
            break;

        case CVAL_SYMBOL:
            free(value->sym);
            break;

        case CVAL_Q_EXPRESSION:
        case CVAL_S_EXPRESSION:
            for (int i = 0; i < value->count; i++) {
                cval_delete(value->cell[i]);
            }
            free(value->cell);
        break;

        case CVAL_STRING:
            free(value->str);
            break;
    }
    free(value);
}

void cenv_delete(cenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->symbols[i]);
        cval_delete(e->values[i]);
    }
    free(e->symbols);
    free(e->values);
    free(e);
}


cval* cval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
    cval_number(x) : cval_error("that'sh an invalid number");
}

cval* cval_add(cval* v, cval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(cval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

cval* cval_read_string(mpc_ast_t* t) {
    t->contents[strlen(t->contents)-1] = '\0';
    char* unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    unescaped = mpcf_unescape(unescaped);
    cval* str = cval_string(unescaped);
    free(unescaped);
    return str;
}

cval* cval_read(mpc_ast_t* t) {

    if (strstr(t->tag, "number")) {
        return cval_read_num(t);
    }

    if (strstr(t->tag, "symbol")) {
        return cval_symbol(t->contents);
    }

    cval* x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = cval_s_expression();
    }
    if (strstr(t->tag, "sexpr")) {
        x = cval_s_expression();
    }
    if (strstr(t->tag, "qexpr")) {
        x = cval_q_expression();
    }
    if (strstr(t->tag, "string")) {
        return cval_read_string(t);
    }

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0){
            continue;
        }
        if (strstr(t->children[i]->tag, "comment")) {
            continue;
        }
        if (strcmp(t->children[i]->contents, ")") == 0){
            continue;
        }
        if (strcmp(t->children[i]->contents, "}") == 0){
            continue;
        }
        if (strcmp(t->children[i]->contents, "{") == 0){
            continue;
        }
        if (strcmp(t->children[i]->tag, "regex") == 0){
            continue;
        }
        x = cval_add(x, cval_read(t->children[i]));
    }
    return x;
}

void cval_expr_print(cval* value, char open, char close) {
    putchar(open);
    for (int i = 0; i < value->count; i++) {
        cval_print(value->cell[i]);
        if (i != (value->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

cval* cval_copy(cval* v);

cenv* cenv_copy(cenv* e) {
    cenv* n = malloc(sizeof(cenv));
    n->par = e->par;
    n->count = e->count;
    n->symbols = malloc(sizeof(char*) * n->count);
    n->values = malloc(sizeof(cval*) * n->count);

    for (int i = 0; i < e->count; i++) {
        n->symbols[i] = malloc(strlen(e->symbols[i]) + 1);
        strcpy(n->symbols[i], e->symbols[i]);
        n->values[i] = cval_copy(e->values[i]);
    }

    return n;
}


cval* cval_copy(cval* v) {

    cval* x = malloc(sizeof(cval));
    x->type = v->type;

    switch (v->type) {

        case CVAL_FUNCTION:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = cenv_copy(v->env);
                x->formals = cval_copy(v->formals);
                x->body = cval_copy(v->body);
            }
            break;

        case CVAL_NUMBER:
            x->num = v->num;
            break;

        case CVAL_ERROR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case CVAL_SYMBOL:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        case CVAL_S_EXPRESSION:
        case CVAL_Q_EXPRESSION:
            x->count = v->count;
            x->cell = malloc(sizeof(cval *) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = cval_copy(v->cell[i]);
            }
            break;

        case CVAL_STRING:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;
    }

    return x;
}

cval* cenv_get(cenv* e, cval* k) {

    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->symbols[i], k->sym) == 0) {
            return cval_copy(e->values[i]);
        }
    }

    if (e->par) {
        return cenv_get(e->par, k);
    } else {
        return cval_error("Unbound shymbol '%s' not defined in shcope!", k->sym);
    }
}

void cenv_put(cenv* e, cval* k, cval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->symbols[i], k->sym) == 0) {
            cval_delete(e->values[i]);
            e->values[i] = cval_copy(v);
            return;
        }
    }

    e->count++;
    e->values = realloc(e->values, sizeof(cval*) * e->count);
    e->symbols = realloc(e->symbols, sizeof(char*) * e->count);

    e->values[e->count-1] = cval_copy(v);
    e->symbols[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->symbols[e->count-1], k->sym);
}

void cenv_def(cenv* e, cval* k, cval* v) {
    while (e->par) {
        e = e->par;
    }
    cenv_put(e, k , v);
}

void cval_print_str(cval* v) {
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void cval_print(cval* value) {
    switch (value->type) {
        case CVAL_NUMBER:
            printf("%li", value->num);
            break;

        case CVAL_FUNCTION:
            if (value->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                cval_print(value->formals);
                putchar(' ');
                cval_print(value->body);
                putchar(')');
            }
            break;

        case CVAL_ERROR:
            printf("shtirred: %s", value->err);
            break;

        case CVAL_SYMBOL:
            printf("%s", value->sym);
            break;

        case CVAL_S_EXPRESSION:
            cval_expr_print(value, '(', ')');
            break;

        case CVAL_Q_EXPRESSION:
            cval_expr_print(value, '{', '}');
            break;

        case CVAL_STRING:
            cval_print_str(value);
            break;
    }
}

void cval_print_line(cval* value) {
    cval_print(value);
    putchar('\n');
}

cval* cval_join(cval* x, cval* y) {
    while (y->count) {
        x = cval_add(x, cval_pop(y, 0));
    }

    cval_delete(y);
    return x;
}

cval* builtin_op(cenv* e, cval* a, char* op) {
    for (int i = 0; i < a->count; i++) {
        CASSERT_TYPE(op, a, i, CVAL_NUMBER);
    }

    cval* x = cval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    while (a->count > 0) {
        cval* y = cval_pop(a, 0);

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

cval* builtin_eval(cenv* e, cval* a) {
    CASSERT_NUM("eval", a, 1);
    CASSERT_TYPE("eval", a, 0, CVAL_Q_EXPRESSION);

    cval* x = cval_take(a, 0);
    x->type = CVAL_S_EXPRESSION;
    return cval_evaluate(e, x);
}

cval* builtin_list(cenv* e, cval* a) {
    a->type = CVAL_Q_EXPRESSION;
    return a;
}

cval* cval_call(cenv* e, cval* f, cval* a) {
    if (f->builtin) {return f->builtin(e, a);}

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {

        if(f->formals->count == 0) {
            cval_delete(a);
            return cval_error("Function pashed too many argumentsh. Got %i, Expected %s", given, total);
        }

        cval* sym = cval_pop(f->formals, 0);

        if (strcmp(sym->sym, "&") == 0) {

            if (f->formals->count != 1) {
                cval_delete(a);
                return cval_error("Function format invalid. shymbol '&' not followed by shingle shymbol.");
            }

            cval* nsym = cval_pop(f->formals, 0);
            cenv_put(f->env, nsym, builtin_list(e, a));
            cval_delete(sym);
            cval_delete(nsym);
            break;
        }

        cval* val = cval_pop(a, 0);
        cenv_put(f->env, sym, val);

        cval_delete(sym);
        cval_delete(val);
    }

    cval_delete(a);

    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {

        if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
            return cval_error("Function format invalid. shymbol '&' not followed by shingle shymbol.");
        }

        cval_delete(cval_pop(f->formals, 0));

        cval* sym = cval_pop(f->formals, 0);
        cval* val = cval_q_expression();

        cenv_put(f->env, sym, val);
        cval_delete(sym);
        cval_delete(val);
    }

    if (f->formals->count == 0) {
        f->env->par = e;
        return builtin_eval(f->env, cval_add(cval_s_expression(), cval_copy(f->body)));
    }
    else {
        return cval_copy(f);
    }
}


cval* cval_evaluate_s_expression(cenv* env, cval* value) {

    for (int i = 0; i < value->count; i++) {
        value->cell[i] = cval_evaluate(env, value->cell[i]);
    }

    for (int i = 0; i < value->count; i++) {
        if (value->cell[i]->type == CVAL_ERROR) {return cval_take(value, i);}
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return cval_evaluate(env, cval_take(value, 0));
    }

    cval* f = cval_pop(value, 0);
    if (f->type != CVAL_FUNCTION) {
        cval* err = cval_error("S-Expression starts with incorrect type. Got %s, Expected %s", ctype_name(f->type), ctype_name(CVAL_FUNCTION));
        cval_delete(f);
        cval_delete(value);
        return err;
    }

    cval* result = cval_call(env, f, value);
    cval_delete(f);
    return result;
}

cval* cval_evaluate(cenv* env, cval* value) {
    if (value->type == CVAL_SYMBOL) {
        cval* x = cenv_get(env, value);
        cval_delete(value);
        return x;
    }

    if (value->type == CVAL_S_EXPRESSION) {
        return cval_evaluate_s_expression(env, value);
    }
    return value;
}

cval* cval_pop(cval* value, int i) {
    cval* x = value->cell[i];
    memmove(&value->cell[i], &value->cell[i+1],
            sizeof(cval*) * (value->count-i-1));

    value->count--;

    value->cell = realloc(value->cell, sizeof(cval*) * value->count);
    return x;
}

cval* cval_take(cval* value, int i) {
    cval* x = cval_pop(value, i);
    cval_delete(value);
    return x;
}

cval* builtin_head(cenv* e, cval* a) {
    CASSERT_NUM("head", a, 1)

    if (a->cell[0]->type == CVAL_STRING) {
        char* first_char;
        first_char[0] = cval_pop(a, 0)->str[0];
        first_char[1] = '\0';
        cval_delete(a);
        return cval_string(first_char);
    }

    if (a->cell[0]->type == CVAL_NUMBER) {
        long n = cval_pop(a, 0)->num;
        long first_digit;

        while(n) {
            first_digit = n;
            n /= 10;
        }
        cval_delete(a);
        return cval_number(first_digit);
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
    CASSERT(a, a->cell[0]->count != 0, "Function 'head' pashed empty list!");

    cval* v = cval_take(a, 0);

    while (v->count > 1) {
        cval_delete((cval_pop(v, 1)));
    }

    return v; }

    cval_delete(a);
    return cval_error("Function 'head' pashed unshupported type!");
}

cval* builtin_tail(cenv* e, cval* a) {
    CASSERT_NUM("tail", a, 1)

    if (a->cell[0]->type == CVAL_STRING) {
        char* str = cval_pop(a, 0)->str;
        cval_delete(a);

        if (strlen(str) > 0) {
            str++;
            return(cval_string(str));
        }
        return(cval_error("Function 'tail' pashed empty shtring!"));
    }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        CASSERT(a, a->cell[0]->count != 0, "Function 'tail' pashed empty list!");

        cval* v = cval_take(a, 0);

        cval_delete(cval_pop(v,0));
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

        switch (init_digits) {
            case 2:
                factor = 10;
                break;
            case 3:
                factor = 100;
                break;
            case 4:
                factor = 1000;
                break;
            case 5:
                factor = 10000;
                break;
            case 6:
                factor = 100000;
                break;
            case 7:
                factor = 1000000;
                break;
            case 8:
                factor = 10000000;
                break;
            case 9:
                factor = 100000000;
                break;
            case 10:
                factor = 1000000000;
                break;
            case 11:
                factor = 10000000000;
                break;
            case 12:
                factor = 100000000000;
                break;
            case 13:
                factor = 1000000000000;
                break;
            case 14:
                factor = 10000000000000;
                break;
            case 15:
                factor = 100000000000000;
                break;
            case 16:
                factor = 1000000000000000;
                break;
            case 17:
                factor = 10000000000000000;
                break;
            case 18:
                factor = 100000000000000000;
                break;
            case 19:
                factor = 1000000000000000000;
                break;
            default:
                return cval_error("Unable to get tail of number.");
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

cval* builtin_rand(cenv* e, cval* a) {
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

cval* builtin_join(cenv* e, cval* a) {

    for (int i = 0; i < a->count; i++) {
        CASSERT_TYPE("join", a, i, CVAL_Q_EXPRESSION)
    }

    cval* x = cval_pop(a, 0);

    while (a->count) {
        x = cval_join(x, cval_pop(a, 0));
    }

    cval_delete(a);

    return x;
}

cval* builtin_add(cenv* e, cval* a) {
    return builtin_op(e, a, "+");
}

cval* builtin_sub(cenv* e, cval* a) {
    return builtin_op(e, a, "-");
}

cval* builtin_mul(cenv* e, cval* a) {
    return builtin_op(e, a, "*");
}

cval* builtin_div(cenv* e, cval* a) {
    return builtin_op(e, a, "/");
}

cval* builtin_modulo(cenv* e, cval* a) {
    return builtin_op(e, a, "mod");
}

cval* builtin_power(cenv* e, cval* a) {
    return builtin_op(e, a, "pow");
}

void cenv_add_builtin(cenv* e, char* name, cbuiltin func) {
    cval* k = cval_symbol(name);
    cval* v = cval_function(func);
    cenv_put(e, k, v);
    cval_delete(k);
    cval_delete(v);
}

cval* builtin_var(cenv* e, cval* a, char* func) {
    CASSERT_TYPE("def", a, 0, CVAL_Q_EXPRESSION)

    cval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        CASSERT_TYPE("def", syms, i, CVAL_SYMBOL)
    }

    CASSERT(a, (syms->count == a->count-1), "Function '%s' pashed too many arguments for symbols. Got %i, Expected %i", func, syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            cenv_def(e, syms->cell[i], a->cell[i+1]);
        }

        if (strcmp(func, "=") == 0) {
            cenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }

    cval_delete(a);
    return cval_s_expression();
}

cval* builtin_put(cenv* e, cval* a) {
    return builtin_var(e, a, "=");
}

cval* builtin_def(cenv* e, cval* a) {
    return builtin_var(e, a, "def");
}

cval* builtin_order(cenv* e, cval* a, char* op) {
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

cval* builtin_greater_than_or_equal(cenv* e, cval* a) {
    return builtin_order(e, a, ">=");
}

cval* builtin_less_than_or_equal(cenv* e, cval* a) {
    return builtin_order(e, a, "<=");
}

cval* builtin_greater_than(cenv* e, cval* a) {
    return builtin_order(e, a, ">");
}

cval* builtin_less_than(cenv* e, cval* a) {
    return builtin_order(e, a, "<");
}

int cval_equal(cval* x, cval* y) {

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

cval* builtin_cmp(cenv* e, cval* a, char* op) {
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

cval* builtin_eq(cenv* e, cval* a) {
    return builtin_cmp(e, a, "==");
}

cval* builtin_ne(cenv* e, cval* a) {
    return builtin_cmp(e, a, "!=");
}

cval* builtin_if (cenv* e, cval* a) {
    CASSERT_NUM("if", a, 3)
    CASSERT_TYPE("if", a, 0, CVAL_NUMBER)
    CASSERT_TYPE("if", a, 1, CVAL_Q_EXPRESSION)
    CASSERT_TYPE("if", a, 2, CVAL_Q_EXPRESSION)

    cval* x;
    a->cell[1]->type = CVAL_S_EXPRESSION;
    a->cell[2]->type = CVAL_S_EXPRESSION;

    if (a->cell[0]->num) {
        x = cval_evaluate(e, cval_pop(a, 1));
    }
    else {
        x = cval_evaluate(e, cval_pop(a, 2));
    }

    cval_delete(a);
    return x;
}

cval* cval_lambda(cval* formals, cval* body) {
    cval* v = malloc(sizeof(cval));
    v->type = CVAL_FUNCTION;

    v->builtin = NULL;

    v->env = cenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

cval* builtin_lambda(cenv* e, cval* a) {
    CASSERT_NUM("lambda", a, 2)
    CASSERT_TYPE("\\", a, 0, CVAL_Q_EXPRESSION);
    CASSERT_TYPE("\\", a, 1, CVAL_Q_EXPRESSION);

    for (int i = 0; i < a->cell[0]->count; i++) {
        CASSERT(a, (a->cell[0]->cell[i]->type == CVAL_SYMBOL), "Cannot define non-symbol. Got %s, Expected %s.", ctype_name(a->cell[0]->cell[i]->type),ctype_name(CVAL_SYMBOL));
    }

    cval* formals = cval_pop(a, 0);
    cval* body = cval_pop(a, 0);
    cval_delete(a);

    return cval_lambda(formals, body);
}

cval* builtin_load(cenv* e, cval* a) {
    CASSERT_NUM("load", a, 1)
    CASSERT_TYPE("load", a, 0, CVAL_STRING)

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Connery, &r)) {
        cval* expr = cval_read(r.output);
        mpc_ast_delete(r.output);

        while (expr->count) {
            cval* x = cval_evaluate(e, cval_pop(expr, 0));

            if (x->type == CVAL_ERROR) {
                cval_print_line(x);
            }
            cval_delete(x);
        }

        cval_delete(expr);
        cval_delete(a);

        return cval_s_expression();
    }
    else {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        cval* err = cval_error("Could not load library '%s'", err_msg);
        free(err_msg);
        cval_delete(a);
        return err;
    }
}

cval* builtin_print(cenv* e, cval* a) {
    for (int i = 0; i < a->count; i++) {
        cval_print(a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    cval_delete(a);

    return cval_s_expression();
}

cval* builtin_error(cenv* e, cval* a) {
    CASSERT_NUM("error", a, 1);
    CASSERT_TYPE("error", a, 0, CVAL_STRING);

    cval* err = cval_error(a->cell[0]->str);

    cval_delete(a);
    return err;
}

cval* builtin_read_file(cenv*e, cval* a) {
    CASSERT_NUM("read_file", a, 1);
    CASSERT_TYPE("read_file", a, 0, CVAL_STRING);

    cval* fileLocation = cval_pop(a, 0);

    FILE *file;
    char *buffer = 0;
    long length;

    file = fopen(fileLocation->str, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        length = ftell (file);
        fseek(file, 0, SEEK_SET);
        buffer = malloc(length);

        if (buffer) {
            fread(buffer, 1, length, file);
        }
        fclose(file);
    }

    if (buffer) {
        return cval_string(buffer);
    }
    else {
        cval_delete(a);
        return cval_error("File not found!");
    }
}

cval* builtin_concat(cenv* e, cval* a) {
    CASSERT_TYPE("concat", a, 0, CVAL_STRING);
    char* newStr = cval_pop(a, 0)->str;
    int count = a->count;

    for (int i = 1; i <= count; i++) {
        CASSERT_TYPE_ITER("concat", a, 0, i, CVAL_STRING)
        newStr = strcat(newStr, cval_pop(a, 0)->str);
    }
    cval_delete(a);
    return cval_string(newStr);
}

cval* builtin_replace(cenv* e, cval* a) {
    CASSERT_TYPE("replace", a, 0, CVAL_STRING);
    CASSERT_TYPE("replace", a, 1, CVAL_STRING);
    CASSERT_TYPE("replace", a, 2, CVAL_STRING);
    CASSERT_TYPE("replace", a, 3, CVAL_NUMBER);
    CASSERT_NUM("replace", a, 4);

    char* str = cval_pop(a, 0)->str;
    char* orig = cval_pop(a, 0)->str;
    char* rep = cval_pop(a, 0)->str;
    long start = cval_pop(a, 0)->num;
    long maxLen = strlen(str) + strlen(rep);

    char temp[maxLen];
    char buffer[maxLen];
    char *p;

    strcpy(temp, str + start);

    if(!(p = strstr(temp, orig))) {
        return cval_string(temp); }

    strncpy(buffer, temp, p-temp);
    buffer[p-temp] = '\0';

    sprintf(buffer + (p - temp), "%s%s", rep, p + strlen(orig));
    sprintf(str + start, "%s", buffer);

    cval_delete(a);
    return cval_string(str);
}

cval* builtin_find(cenv* e, cval* a) {
    CASSERT_TYPE("find", a, 0, CVAL_STRING);
    CASSERT_TYPE("find", a, 1, CVAL_STRING);
    CASSERT_NUM("find", a, 2);

    char* pos;
    char* org = cval_pop(a, 0)->str;

    if ((pos = strstr(org, cval_pop(a, 0)->str))) {
        cval_delete(a);
        return cval_number(pos - org);
    }
    else {
        cval_delete(a);
        return cval_number(-1);
    }
}

cval* builtin_split(cenv* e, cval* a) {
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

    char first_half[split_index] ;
    char second_half[str_length - split_index];

    for(int i = 0; i <= str_length; ++i)
    {
        if (i < split_index) {
            first_half[i] = original_string[i];
            first_half[i+1] = '\0';
        }
        else {
            second_half[i - split_index] = original_string[i];
        }
    }

    cval* list = cval_q_expression();
    cval* first = cval_string(first_half);
    cval* second = cval_string(second_half);

    cval_add(list, first);
    cval_add(list, second);

    cval_delete(a);

    return list;
}

cval* builtin_chop(cenv* e, cval* a) {
    CASSERT_TYPE("chop", a, 0, CVAL_STRING);
    CASSERT_TYPE("chop", a, 1, CVAL_NUMBER);
    CASSERT_NUM("chop", a, 2);

    char *original_string = cval_pop(a, 0)->str;
    long chars_to_chop = cval_pop(a, 0)->num;

    if (strlen(original_string) > chars_to_chop) {
        cval_delete(a);
        return cval_string(original_string + chars_to_chop);
    }
    else {
        cval_delete(a);
        return cval_string("");
    }
}

cval* builtin_length(cenv* e, cval* a) {
    CASSERT_NUM("length", a, 1);

    if (a->cell[0]->type == CVAL_NUMBER) {
        long length = count_digits(a->cell[0]->num);
        cval_delete(a);
        return cval_number(length);
    }

    if (a->cell[0]->type == CVAL_STRING) {
        long length = strlen(cval_pop(a, 0)->str);
        cval_delete(a);
        return cval_number(length); }

    if (a->cell[0]->type == CVAL_Q_EXPRESSION) {
        long length = a->cell[0]->count;
        cval_delete(a);
        return cval_number(length);
    }

    cval_delete(a);
    return cval_error("Function 'length' pashed unshupported type!");
}

cval* builtin_string_case(cenv* e, cval* a) {
    CASSERT_TYPE("case", a, 0, CVAL_NUMBER);
    CASSERT_TYPE("case", a, 1, CVAL_STRING);
    CASSERT_NUM("case", a, 2);

    long case_modifier = cval_pop(a, 0)->num;
    char *org_string = cval_pop(a, 0)->str;
    char cased_string[strlen(org_string) + 1];

    cval_delete(a);

    if (case_modifier >= 2) {
        return cval_error("Unknown argument %i for case, only 0 (lowercase) and 1 (uppercase) are accepted.", case_modifier);
    }

    if (case_modifier == 0) {
        for(int i = 0; org_string[i]; i++){
            cased_string[i] = tolower(org_string[i]);
            cased_string[i + 1] = '\0';}
    } else {
        for(int i = 0; org_string[i]; i++){
            cased_string[i] = toupper(org_string[i]);
            cased_string[i + 1] = '\0';}
    }

    return cval_string(cased_string);
}

cval* builtin_type(cenv* e, cval* a) {
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

void instantiate_string_builtins(cenv* e) {
    cenv_add_builtin(e, "concat", builtin_concat);
    cenv_add_builtin(e, "replace", builtin_replace);
    cenv_add_builtin(e, "find", builtin_find);
    cenv_add_builtin(e, "split", builtin_split);
    cenv_add_builtin(e, "string_case", builtin_string_case);
    cenv_add_builtin(e, "chop", builtin_chop);
    cenv_add_builtin(e, "length", builtin_length);
}

void cenv_add_builtins(cenv* e) {
    cenv_add_builtin(e, "\\",  builtin_lambda);
    cenv_add_builtin(e, "def", builtin_def);
    cenv_add_builtin(e, "=",   builtin_put);

    cenv_add_builtin(e, "list", builtin_list);
    cenv_add_builtin(e, "head", builtin_head);
    cenv_add_builtin(e, "tail", builtin_tail);
    cenv_add_builtin(e, "eval", builtin_eval);
    cenv_add_builtin(e, "join", builtin_join);
    cenv_add_builtin(e, "def", builtin_def);

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

    cenv_add_builtin(e, "read_file", builtin_read_file);
    instantiate_string_builtins(e);
}

void load_standard_lib(cenv* e) {
    cval* stdLib = cval_add(cval_s_expression(), cval_string("stdlib/main.connery"));
    if (stdLib->type == CVAL_ERROR) {
        cval_print_line(stdLib);
    }
    builtin_load(e, stdLib);
}

int main(int argc, char** argv) {
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Comment = mpc_new("comment");
    String = mpc_new("string");
    Connery = mpc_new("connery");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                 \
                number    : /-?[0-9]+/ ;                              \
                symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/          \
                            |'+' | '-' | '*' | '/' ;                   \
                sexpr     : '(' <expr>* ')' ;                         \
                qexpr     : '{' <expr>* '}' ;                         \
                string  : /\\\"(\\\\\\\\.|[^\\\"])*\\\"/ ;             \
                comment : /;[^\\r\\n]*/  ;                              \
                expr      : <number> | <symbol> | <comment> | <sexpr> | <qexpr> | <string> ; \
                connery   : /^/ <expr>* /$/ ;                          \
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, String, Comment, Connery);

    cenv* e = cenv_new();
    cenv_add_builtins(e);
    load_standard_lib(e);

    puts("   ______                                 \n"
         "  / ____/___  ____  ____  ___  _______  __\n"
         " / /   / __ \\/ __ \\/ __ \\/ _ \\/ ___/ / / /\n"
         "/ /___/ /_/ / / / / / / /  __/ /  / /_/ / \n"
         "\\____/\\____/_/ /_/_/ /_/\\___/_/   \\__, /  \n"
         "                                 /____/   ");
    puts("+-----      Version 0.0.4      ------+\n");

    if (argc == 1) {
        while (1) {
            char* input = readline("connery> ");
            add_history(input);

            mpc_result_t result;
            if (mpc_parse("<stdin>", input, Connery, &result)) {
                cval* output = cval_evaluate(e, cval_read(result.output));
                cval_print_line(output);
                cval_delete(output);

                mpc_ast_delete(result.output);
            }
            else {
                mpc_err_print(result.error);
                mpc_err_delete(result.error);
            }
            free(input);
        }
    }

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {

            cval* args = cval_add(cval_s_expression(), cval_string(argv[i]));
            cval* x = builtin_load(e, args);

            if (x->type == CVAL_ERROR) {
                cval_print_line(x);
            }
            cval_delete(x);
        }
    }

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Connery);
    cenv_delete(e);
    return 0;
}