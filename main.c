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

enum {CVAL_NUMBER, CVAL_ERROR};
enum {CERR_DIVISION_BY_ZERO, CERR_BAD_OPERATOR, CERR_BAD_NUMBER};

typedef struct {
    int type;
    long num;
    int err;
} cval;

cval cval_number(long x) {
    cval value;
    value.type = CVAL_NUMBER;
    value.num = x;
    return value;
}

cval cval_error(int x) {
    cval value;
    value.type = CVAL_ERROR;
    value.err = x;
    return value;
}

void cval_print(cval value) {
    switch (value.type) {

        case CVAL_NUMBER:
            printf("%li", value.num);
            break;

        case CVAL_ERROR:
            switch (value.err) {
                case CERR_DIVISION_BY_ZERO:
                    printf("Error: Division by zero.");
                    break;

                case CERR_BAD_OPERATOR:
                    printf("Error: Unknown operator.");
                    break;

                case CERR_BAD_NUMBER:
                    printf("Error: Invalid number.");
                    break;
            }
    break;
    }
}

void cval_print_line(cval value) {
    cval_print(value);
    putchar('\n');
}


cval eval_op(cval x, char* op, cval y) {
    if (x.type == CVAL_ERROR) {return x;}
    if (y.type == CVAL_ERROR) {return y;}

    if (strcmp(op, "+") == 0) {return cval_number(x.num + y.num);}
    if (strcmp(op, "-") == 0) {return cval_number(x.num - y.num);}
    if (strcmp(op, "*") == 0) {return cval_number(x.num * y.num);}
    if (strcmp(op, "/") == 0) {
    return y.num == 0
    ? cval_error(CERR_DIVISION_BY_ZERO)
    : cval_number(x.num / y.num);}

    return cval_error(CERR_BAD_OPERATOR);
}

cval eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? cval_number(x) : cval_error(CERR_BAD_NUMBER);
    }

    char* op = t->children[1]->contents;
    cval x = eval(t->children[2]);
    int i = 3;

    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    return x;
}

int main() {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Connery = mpc_new("connery");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                \
                number    : /-?[0-9]+/ ;                             \
                operator  : '+' | '-' | '*' | '/' ;                  \
                expr      : <number> | '(' <operator>  <expr>+ ')' ; \
                connery   : /^/ <operator> <expr>+ /$/ ;             \
            ",
            Number, Operator, Expr, Connery);



    puts("Connery version 0.0.1");
    puts("Press Ctrl + c to exit\n");

    while (1) {
        char* input = readline("connery> ");
        add_history(input);

        mpc_result_t result;
        mpc_parse("<stdin>", input, Connery, &result);

        cval output = eval(result.output);
        cval_print_line(output);
        mpc_ast_delete(result.output);

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Connery);
    return 0;
}
