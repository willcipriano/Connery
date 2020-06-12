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

        if (mpc_parse("<stdin>", input, Connery, &result)) {
            mpc_ast_print(result.output);
            mpc_ast_delete(result.output);
        } else {
            mpc_err_print(result.error);
            mpc_err_delete(result.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Connery);
    return 0;
}
