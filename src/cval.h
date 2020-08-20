#ifndef CONNERY_CVAL_H
#define CONNERY_CVAL_H
#include "mpc.h"
typedef struct cval cval;
typedef struct cenv cenv;
typedef cval*(*cbuiltin)(cenv*, cval*);

enum {CVAL_NUMBER, CVAL_ERROR, CVAL_SYMBOL, CVAL_FUNCTION,
    CVAL_S_EXPRESSION, CVAL_Q_EXPRESSION, CVAL_STRING, CVAL_FLOAT};

struct cval {
    int type;

    long num;
    long double fnum;
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

char* ctype_name(int t);
cval* cval_function(cbuiltin func);
cval* cval_number(long x);
cval* cval_float(long double x);
cval* cval_string (char* s);
cval* cval_error(char* fmt, ...);
cval* cval_symbol(char* s);
cval* cval_s_expression(void);
cval* cval_q_expression(void);
cenv* cenv_new(void);
void cenv_delete(cenv* e);
void cval_delete(cval* value);
cval* cval_take(cval* value, int i);
cval* cval_pop(cval* value, int i);
cval* builtin_eval(cenv* e, cval* a);
cenv* cenv_copy(cenv* e);
cval* cval_copy(cval* v);
void cenv_put(cenv* e, cval* k, cval* v);
cval* builtin_list(cenv* e, cval* a);
cval* cval_add(cval* v, cval* x);
cval* cval_call(cenv* e, cval* f, cval* a);
cval* cval_evaluate_s_expression(cenv* env, cval* value);
cval* cenv_get(cenv* e, cval* k);
cval* cval_evaluate(cenv* env, cval* value);
void cenv_add_builtin(cenv* e, char* name, cbuiltin func);
void cenv_def(cenv* e, cval* k, cval* v);
cval* builtin_var(cenv* e, cval* a, char* func);
cval* builtin_put(cenv* e, cval* a);
cval* builtin_def(cenv* e, cval* a);
cval* cval_join(cval* x, cval* y);
cval* cval_read_num(mpc_ast_t* t);
cval* cval_read_float(mpc_ast_t* t);
cval* cval_read(mpc_ast_t* t);
void cval_print_line(cval* value);
void cval_expr_print(cval* value, char open, char close);
void cval_print(cval* value);
#endif //CONNERY_CVAL_H
