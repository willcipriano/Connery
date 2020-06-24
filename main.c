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

enum {CVAL_NUMBER, CVAL_ERROR, CVAL_SYMBOL, CVAL_S_EXPRESSION, CVAL_Q_EXPRESSION};
enum {CERR_DIVISION_BY_ZERO, CERR_BAD_OPERATOR, CERR_BAD_NUMBER};

typedef struct cval {
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct cval** cell;
} cval;

cval* cval_number(long x) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_NUMBER;
    value->num = x;
    return value;
}

cval* cval_error(char* s) {
    cval* value = malloc(sizeof(cval));
    value->type = CVAL_ERROR;
    value->err = malloc(strlen(s) + 1);
    strcpy(value->err, s);
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

void cval_delete(cval* value) {
    switch(value->type) {
        case CVAL_NUMBER:
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

void cval_print(cval* value);

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


void cval_print(cval* value) {
    switch (value->type) {

        case CVAL_NUMBER:
            printf("%li", value->num);
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

cval* cval_evaluate(cval* value);

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

cval* builtin_op(cval* a, char* op) {

    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != CVAL_NUMBER) {
            cval_delete(a);
            return cval_error("cannot operate on anything but numbersh");
        }
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


cval* cval_evaluate_s_expression(cval* value) {

    for (int i = 0; i < value->count; i++) {
        value->cell[i] = cval_evaluate(value->cell[i]);
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
    if (f->type != CVAL_SYMBOL) {
        cval_delete(f);
        cval_delete(value);
        return cval_error("sh-expreshion doesh not shtart with a shymbol!");
    }

    cval* result = builtin_op(value, f->sym);
    cval_delete(f);
    return result;
}

cval* cval_evaluate(cval* value) {
    if (value->type == CVAL_S_EXPRESSION) {
        return cval_evaluate_s_expression(value);
    }
    return value;
}

//cval eval_op(cval x, char* op, cval y) {
//    if (x.type == CVAL_ERROR) {return x;}
//    if (y.type == CVAL_ERROR) {return y;}
//
//    if (strcmp(op, "+") == 0) {return cval_number(x.num + y.num);}
//    if (strcmp(op, "-") == 0) {return cval_number(x.num - y.num);}
//    if (strcmp(op, "*") == 0) {return cval_number(x.num * y.num);}
//    if (strcmp(op, "/") == 0) {
//    return y.num == 0
//    ? cval_error(CERR_DIVISION_BY_ZERO)
//    : cval_number(x.num / y.num);}
//
//    return cval_error(CERR_BAD_OPERATOR);
//}

//cval eval(mpc_ast_t* t) {
//    if (strstr(t->tag, "number")) {
//        errno = 0;
//        long x = strtol(t->contents, NULL, 10);
//        return errno != ERANGE ? cval_number(x) : cval_error(CERR_BAD_NUMBER);
//    }
//
//    char* op = t->children[1]->contents;
//    cval x = eval(t->children[2]);
//    int i = 3;
//
//    while(strstr(t->children[i]->tag, "expr")) {
//        x = eval_op(x, op, eval(t->children[i]));
//        i++;
//    }
//    return x;
//}

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
                symbol    : '+' | '-' | '*' | '/' ;                   \
                sexpr     : '(' <expr>* ')' ;                         \
                qexpr     : '{' <expr>* '}' ;                         \
                expr      : <number> | <symbol> | <sexpr> | <qexpr> ; \
                connery   : /^/ <expr>* /$/ ;                          \
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, Connery);


    puts("Connery version 0.0.1");
    puts("Press Ctrl + c to exit\n");

    while (1) {
        char* input = readline("connery> ");
        add_history(input);

        mpc_result_t result;
        mpc_parse("<stdin>", input, Connery, &result);

        cval* output = cval_evaluate(cval_read(result.output));
        cval_print_line(output);
        cval_delete(output);

        free(input);
    }

    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Connery);
    return 0;
}
