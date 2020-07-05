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

enum {CVAL_NUMBER, CVAL_ERROR, CVAL_SYMBOL, CVAL_FUNCTION, CVAL_S_EXPRESSION, CVAL_Q_EXPRESSION};

typedef cval*(*cbuiltin)(cenv*, cval*);
void cenv_delete(cenv* e);

char* ctype_name(int t) {
    switch(t) {
        case CVAL_FUNCTION: return "Function";
        case CVAL_NUMBER: return "Number";
        case CVAL_ERROR: return "Error";
        case CVAL_SYMBOL: return "Symbol";
        case CVAL_S_EXPRESSION: return "S-Expression";
        case CVAL_Q_EXPRESSION: return "Q-Expression";
        default: return "Unknown Type";
    }
}

struct cval {
    int type;

    long num;
    char* err;
    char* sym;

    cbuiltin builtin;
    cenv* env;
    cval* formals;
    cval* body;

    int count;
    cval** cell;
};

struct cenv {
    int count;
    char** symbols;
    cval** values;
};

#define CASSERT(args, cond, fmt, ...) \
if (!(cond)) {\
    cval* err = cval_error(fmt, ##__VA_ARGS__); \
    cval_delete(args); \
    return err;}

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
            if (value->builtin) {
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

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0){
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

//    todo: optimize function defs
void cval_print(cval* value);
cval* cval_pop(cval* value, int i);
cval* cval_evaluate(cenv* env,cval* value);
cval* cval_take(cval* value, int i);

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


cval* cval_copy(cval* v) {

    cval* x = malloc(sizeof(cval));
    x->type = v->type;

    switch (v->type) {

        case CVAL_FUNCTION:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env =
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

    }

    return x;
}

cval* cenv_get(cenv* e, cval* k) {
    for (int i = 0; i < e->count; i++) {

        if (strcmp(e->symbols[i], k->sym) == 0) {
            return cval_copy(e->values[i]);
        }
    }

    return cval_error("Unbound shymbol '%s' not defined in shcope!", k->sym);
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

void cval_print(cval* value) {
    switch (value->type) {

        case CVAL_NUMBER:
            printf("%li", value->num);
            break;

        case CVAL_FUNCTION:
            printf("<function>");
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
        CASSERT(a, a->cell[i]->type == CVAL_NUMBER, "Function '%s' passed incorrect type for argument %i Got %s, Expected %s.", op, i, ctype_name(a->cell[i]->type), ctype_name(CVAL_NUMBER));
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
        cval_delete(y);
    }
    cval_delete(a);
    return x;
}


cval* cval_evaluate_s_expression(cenv* env, cval* value) {

    for (int i = 0; i < value->count; i++) {
        value->cell[i] = cval_evaluate(env, value->cell[i]);
    }

    for (int i = 0; i < value->count; i++) {
        if (value->cell[i]->type == CVAL_ERROR) {return cval_take(value, 0);}
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return cval_take(value, 0);
    }

    cval* f = cval_pop(value, 0);
    if (f->type != CVAL_FUNCTION) {
        cval_delete(f);
        cval_delete(value);
        return cval_error("firsht element ish not a function!");
    }

    cval* result = f->builtin(env, value);
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
    CASSERT(a, a->count==1, "Function 'head' pashed in too many argumentsh! Got %i, Expected %i!", a->count, 1);
    CASSERT(a, a->cell[0]->type == CVAL_Q_EXPRESSION, "Function 'head' pashed incorrect typesh! Got %s, Expected %s", ctype_name(a->cell[0]->type), ctype_name(CVAL_Q_EXPRESSION));
    CASSERT(a, a->cell[0]->count != 0, "Function 'head' pashed empty list!");

    cval* v = cval_take(a, 0);

    while (v->count > 1) {
        cval_delete((cval_pop(v, 1)));
    }

    return v;
}

cval* builtin_tail(cenv* e, cval* a) {
    CASSERT(a, a->count==1, "Function 'tail' pashed in too many argumentsh! Got %i, Expected %i!", a->count, 1);
    CASSERT(a, a->cell[0]->type == CVAL_Q_EXPRESSION, "Function 'tail' pashed incorrect typesh! Got %s, Expected %s", ctype_name(a->cell[0]->type), ctype_name(CVAL_Q_EXPRESSION));
    CASSERT(a, a->cell[0]->count != 0, "Function 'tail' pashed empty list!");

    cval* v = cval_take(a, 0);

    cval_delete(cval_pop(v,0));
    return v;
}

cval* builtin_list(cenv* e, cval* a) {
    a->type = CVAL_Q_EXPRESSION;
    return a;
}

cval* builtin_eval(cenv* e, cval* a) {
    CASSERT(a, a->count == 1, "Function 'eval' pashed in too many argumentsh! Got %i, Expected %i!", a->count, 1)
    CASSERT(a, a->cell[0]->type == CVAL_Q_EXPRESSION, "Function 'eval' pashed incorrect typesh! Got %s, Expected %s", ctype_name(a->cell[0]->type), ctype_name(CVAL_Q_EXPRESSION))

    cval* x = cval_take(a, 0);
    x->type = CVAL_S_EXPRESSION;
    return cval_evaluate(e, x);
}

cval* builtin_join(cenv* e, cval* a) {

    for (int i = 0; i < a->count; i++) {
        CASSERT(a, a->cell[i]->type == CVAL_Q_EXPRESSION, "Function 'join' pashed incorrect typesh! Got %s, Expected %s", ctype_name(a->cell[0]->type), ctype_name(CVAL_Q_EXPRESSION))
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

void cenv_add_builtin(cenv* e, char* name, cbuiltin func) {
    cval* k = cval_symbol(name);
    cval* v = cval_function(func);
    cenv_put(e, k, v);
    cval_delete(k);
    cval_delete(v);
}

cval* builtin_def(cenv* e, cval* a) {
    CASSERT(a, a->cell[0]->type == CVAL_Q_EXPRESSION, "Function 'def' pashed incorrect type! Got %s, Expected %s", ctype_name(a->cell[0]->type), ctype_name(CVAL_Q_EXPRESSION));
    cval* syms = a->cell[0];

    for (int i = 0; i < syms->count; i++) {
        CASSERT(a, syms->cell[i]->type == CVAL_SYMBOL, "Function 'def' cannot define non-shymbol!");
    }

    CASSERT(a, syms->count == a->count-1, "Function 'def' cannot define incorrect number of valuesh to shymbolsh! Got %i, Expected %i", syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++) {
        cenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    cval_delete(a);
    return cval_s_expression();
}

void cenv_add_builtins(cenv* e) {
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

int main() {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Connery = mpc_new("connery");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                 \
                number    : /-?[0-9]+/ ;                              \
                symbol    : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/          \
                            |'+' | '-' | '*' | '/' ;                   \
                sexpr     : '(' <expr>* ')' ;                         \
                qexpr     : '{' <expr>* '}' ;                         \
                expr      : <number> | <symbol> | <sexpr> | <qexpr> ; \
                connery   : /^/ <expr>* /$/ ;                          \
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, Connery);

    cenv* e = cenv_new();
    cenv_add_builtins(e);

    puts("Connery version 0.0.1");
    puts("Press Ctrl + c to exit\n");

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

    cenv_delete(e);
}
