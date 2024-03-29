#include "cval.h"
#include "util.h"
#include "trace.h"
#include "hashtable.h"
#include "allocator.h"
#include "http.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "globals.h"

#define CASSERT(args, cond, fmt, ...) \
if (!(cond)) {\
    cval* err = cval_fault(fmt, ##__VA_ARGS__); \
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


cval* NULL_CVAL_CONSTANT = NULL;
cval* TRUE_CVAL_CONSTANT = NULL;
cval* FALSE_CVAL_CONSTANT = NULL;
bool IMMORTALS_CREATED = false;

char* ctype_name(int t) {
    switch(t) {
        case CVAL_FUNCTION: return "Function";
        case CVAL_NUMBER: return "Number";
        case CVAL_FAULT: return "Fault";
        case CVAL_SYMBOL: return "Symbol";
        case CVAL_S_EXPRESSION: return "S-Expression";
        case CVAL_Q_EXPRESSION: return "Q-Expression";
        case CVAL_STRING: return "String";
        case CVAL_FLOAT: return "Float";
        case CVAL_BOOLEAN: return "Boolean";
        case CVAL_DICTIONARY: return "Dictionary";
        case CVAL_NULL: return "Null";
        case CVAL_UNALLOCATED: return "UNALLOCATED";
        case CVAL_REALLOCATED: return "REALLOCATED";
        default: return "Unknown Type";
    }
}

cval* cval_function(cbuiltin func) {
    cval* v = allocate();
    v->type = CVAL_FUNCTION;
    v->builtin = func;
    return v;
}

void create_immortals() {
    if (!IMMORTALS_CREATED) {
    cval *nullConst = allocate();
    nullConst->type = CVAL_NULL;
    nullConst->count = -1;
    nullConst->str = "NULL";
    nullConst->num = 0;
    nullConst->fnum = 0.0;
    nullConst->cell = NULL;
    nullConst->boolean = false;
    NULL_CVAL_CONSTANT = nullConst;

    cval *trueConst = allocate();
    trueConst->type = CVAL_BOOLEAN;
    trueConst->num = 1;
    trueConst->fnum = 1;
    trueConst->boolean = true;
    trueConst->count = 0;
    trueConst->cell = NULL;
    TRUE_CVAL_CONSTANT = trueConst;

    cval *falseConst = allocate();
    falseConst->type = CVAL_BOOLEAN;
    falseConst->num = -1;
    falseConst->fnum = -1.0;
    falseConst->boolean = false;
    falseConst->count = 0;
    falseConst->cell = NULL;
    FALSE_CVAL_CONSTANT = falseConst;}

    IMMORTALS_CREATED = true;
}

cval *cval_boolean(bool b) {
    if (!IMMORTALS_CREATED) {
        create_immortals();
    }

    if (b) {
        return TRUE_CVAL_CONSTANT;
    }

    return FALSE_CVAL_CONSTANT;
}

cval* cval_number(long x) {
    cval* value = allocate();
    value->type = CVAL_NUMBER;
    value->num = x;
    return value;
}

cval* cval_float(long double x) {
    cval* value = allocate();
    value->type = CVAL_FLOAT;
    value->fnum = x;
    return value;
}

cval* cval_string (char* s) {
    cval* v = allocate();
    v->type = CVAL_STRING;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

cval* cval_fault(char* fmt, ...) {
    cval* value = allocate();
    value->type = CVAL_FAULT;
    va_list va;
    va_start(va, fmt);
    value->err = malloc(512);
    vsnprintf(value->err, 511, fmt, va);
    value->err = realloc(value->err, strlen(value->err)+1);
    va_end(va);
    return value;
}

cval* cval_symbol(char* s) {
    cval* value = allocate();
    value->type = CVAL_SYMBOL;
    value->sym = malloc(strlen(s) + 1);
    strcpy(value->sym, s);
    return value;
}

cval* cval_s_expression(void) {
    cval* value = allocate();
    value->type = CVAL_S_EXPRESSION;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cval* cval_q_expression(void) {
    cval* value = allocate();
    value->type = CVAL_Q_EXPRESSION;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cval* cval_dictionary(hash_table* ht) {
    cval* value = allocate();
    value->type = CVAL_DICTIONARY;
    value->count = ht->items;
    value->ht = ht;
    value->cell = NULL;
    return value;
}

cval* cval_null() {
    if (!IMMORTALS_CREATED) {
        create_immortals();
    }

    return NULL_CVAL_CONSTANT;
}

cenv* cenv_new(void) {
    cenv* e = malloc(sizeof(cenv));
    e->par = NULL;
    e->ht = hash_table_create(ENV_HASH_TABLE_SIZE);
    return e;
}

void cenv_delete(cenv* e) {
    hash_table_destroy(e->ht);
    free(e);
}

void cval_delete(cval* value) {
    bool immortal = false;
    switch(value->type) {

        case CVAL_NULL:
        case CVAL_BOOLEAN:
            immortal = true;
            break;

        case CVAL_FUNCTION:
            if (!value->builtin) {
                cenv_delete(value->env);
                cval_delete(value->formals);
                cval_delete(value->body);
            }
            break;

        case CVAL_Q_EXPRESSION:
        case CVAL_S_EXPRESSION:
            for (int i = 0; i < value->count; i++) {
                cval_delete(value->cell[i]);
            }
            free(value->cell);
            break;
    }

    if (!immortal) {
        deallocate(value); }
}

cval* cval_take(cval* value, int i) {
    cval* x = cval_pop(value, i);
    cval_delete(value);
    return x;
}

cval* cval_pop(cval* value, int i) {
    cval* x = value->cell[i];
    memmove(&value->cell[i], &value->cell[i+1],
            sizeof(cval*) * (value->count-i-1));

    value->count--;

    value->cell = realloc(value->cell, sizeof(cval*) * value->count);
    return x;
}

cval* builtin_eval(cenv* e, cval* a) {
    CASSERT_NUM("eval", a, 1);
    CASSERT_TYPE("eval", a, 0, CVAL_Q_EXPRESSION);

    cval* x = cval_take(a, 0);
    x->type = CVAL_S_EXPRESSION;
    return cval_evaluate(e, x);
}

cenv* cenv_copy(cenv* e) {
    cenv* n = malloc(sizeof(cenv));
    n->par = e->par;
    n->ht = hash_table_copy(e->ht);
    return n;
}

cval* cval_copy(cval* v) {

    if (v == NULL || v->type == CVAL_NULL) {
        return cval_null();
    }

    cval* x = allocate();
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

        case CVAL_FLOAT:
            x->fnum = v->fnum;
            break;

        case CVAL_FAULT:
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
            x->cell = calloc(sizeof(cval*), x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = cval_copy(v->cell[i]);
            }
            break;

        case CVAL_STRING:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;

        case CVAL_BOOLEAN:
            x->boolean = v->boolean;
            break;

        case CVAL_DICTIONARY:
            x->ht = v->ht;
            break;
    }

    return x;
}

void cenv_put(cenv* e, cval* k, cval* v) {
    hash_table_set(e->ht, k->sym, v);
}

cval* builtin_list(cenv* e, cval* a) {
    a->type = CVAL_Q_EXPRESSION;
    return a;
}

cval* cval_add(cval* v, cval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(cval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

cval* cval_call(cenv* e, cval* f, cval* a) {
    if (f->builtin) {return f->builtin(e, a);}

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {

        if(f->formals->count == 0) {
            cval_delete(a);
#if SYSTEM_LANG == 0
            return cval_fault("Function pashed too many argumentsh. Got %i, Expected %i", given, total);
#else
            return cval_fault("Function passed too many arguments. Got %i, Expected %i", given, total);
#endif
        }

        cval* sym = cval_pop(f->formals, 0);

        if (strcmp(sym->sym, "&") == 0) {

            if (f->formals->count != 1) {
                cval_delete(a);
#if SYSTEM_LANG == 0
                return cval_fault("Function format invalid. shymbol '&' not followed by shingle shymbol.");
#else
                return cval_fault("Function format invalid. Symbol '&' not followed by a single symbol.");
#endif
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
#if SYSTEM_LANG == 0
            return cval_fault("Function format invalid. shymbol '&' not followed by shingle shymbol.");
#else
            return cval_fault("Function format invalid. Symbol '&' not followed by a single symbol.");
#endif
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
        if (value->cell[i]->type == CVAL_FAULT) {return cval_take(value, i);}
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return cval_evaluate(env, cval_take(value, 0));
    }

    cval* f = cval_pop(value, 0);
    if (f->type != CVAL_FUNCTION) {
            if (f->type == CVAL_S_EXPRESSION) {
                return cval_evaluate_s_expression(env, f);
            }
            else {
#if SYSTEM_LANG == 0
                return cval_fault("I'm afraid thatsh not valid connery, lad.");
#else
                return cval_fault("Invalid input");
#endif
            }
        }

    cval* result = cval_call(env, f, value);
    cval_delete(f);
    return result;
}

cval* cenv_get(cenv* e, cval* k) {
    cval* value = hash_table_get(e->ht, k->sym);
    if (value != NULL) {
        return cval_copy(value);
    }

    if (!e->par) {
#if SYSTEM_LANG == 0
        return cval_fault("Unbound shymbol '%s' not defined in shcope!", k->sym);
#else
        return cval_fault("Unbound symbol '%s' not defined in scope!", k->sym);
#endif
    }
    return cenv_get(e->par, k);
}

cval* safe_cenv_get(cenv* e, char * s) {
    cval* value = hash_table_get(e->ht, s);
    if (value != NULL) {
        return cval_copy(value);
    }

    if (!e->par) {
       return NULL;
    }

    return safe_cenv_get(e->par, s);
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

void cenv_add_builtin(cenv* e, char* name, cbuiltin func) {
    cval* k = cval_symbol(name);
    cval* v = cval_function(func);
    cenv_put(e, k, v);
    cval_delete(k);
    cval_delete(v);
}


void cenv_def(cenv* e, cval* k, cval* v) {
    while (e->par) {
        e = e->par;
    }
    cenv_put(e, k , v);
}

cval* builtin_var(cenv* e, cval* a, char* func) {
    CASSERT_TYPE("def", a, 0, CVAL_Q_EXPRESSION)

    cval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        CASSERT_TYPE("def", syms, i, CVAL_SYMBOL)
    }

    CASSERT(a, (syms->count == a->count - 1),
            "Function '%s' pashed too many arguments for symbols. Got %i, Expected %i", func, syms->count, a->count - 1);

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

cval* cval_join(cval* x, cval* y) {
    for (int i = 0; i < y->count; i++) {
        x = cval_add(x, y->cell[i]);
    }

    for (int i = 0; i < y->count; i++) {
        deallocate(y->cell[i]);
    }

    deallocate(y);
    return x;
}

cval *cval_read_boolean(mpc_ast_t *t) {
    if (strstr(t->contents, "True")) {
        return cval_boolean(true);
    }
    return cval_boolean(false);
}

cval* cval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
#if SYSTEM_LANG == 0
    return errno != ERANGE ?
           cval_number(x) : cval_fault("that'sh an invalid number");
#else
    return errno != ERANGE ?
           cval_number(x) : cval_fault("that's an invalid number");
#endif


}

cval* cval_read_float(mpc_ast_t* t) {
    errno = 0;
    long double x = strtold(t->contents, NULL);
#if SYSTEM_LANG == 0
    return errno != ERANGE ?
           cval_float(x) : cval_fault("that'sh a invalid float");
#else
    return errno != ERANGE ?
           cval_float(x) : cval_fault("that's a invalid float");
#endif
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

cval *cval_read_symbol(char *symbol) {
        return cval_symbol(symbol);
}

cval *cval_read_dictionary(mpc_ast_t *t) {
    int items = t->children_num - 2;
    int iter = 1;

    hash_table* ht = hash_table_create(items + DICTIONARY_LITERAL_INSTANTIATED_HASH_TABLE_MINIMUM);

    while (iter <= items) {
        t->children[iter]->children[0]->contents[strlen(t->children[iter]->children[0]->contents)-1] = '\0';
        char* unescaped = malloc(strlen(t->children[iter]->children[0]->contents+1)+1);
        strcpy(unescaped, t->children[iter]->children[0]->contents+1);
        unescaped = mpcf_unescape(unescaped);
        hash_table_set(ht, unescaped, cval_read(t->children[iter]->children[2]));
        free(unescaped);
        iter += 1;
    }

    return cval_dictionary(ht);
}

cval* cval_read(mpc_ast_t* t) {

    if (strstr(t->tag, "boolean")) {
        return cval_read_boolean(t);
    }

    if (strstr(t->tag, "number")) {
        return cval_read_num(t);
    }

    if (strstr(t->tag, "float")) {
        return cval_read_float(t);
    }

    if (strstr(t->tag, "symbol")) {
        return cval_read_symbol(t->contents);
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
    if (strstr(t->tag, "dictionary")) {
        return cval_read_dictionary(t);
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


void cval_print_str(cval* v) {
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("%s", v->str);
    free(escaped);
}

void cval_print_ht_str(cval* v, char* key) {
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("%s : \"%s\"", key, escaped);
    free(escaped);
}

bool cval_expr_print(cval* value, char open, char close) {
    if (value->count >= 1) {
    putchar(open);
    for (int i = 0; i < value->count; i++) {
        cval_print(value->cell[i]);
        if (i != (value->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
    return true;}
    return false;
}

void cval_expr_ht_print(cval* value, char open, char close, char* key) {
    fputs(key, stdout);
    fputs(" : ", stdout);
    putchar(open);
    for (int i = 0; i < value->count; i++) {
        cval_print(value->cell[i]);
        if (i != (value->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}


bool cval_print(cval* value) {
    switch (value->type) {

        case CVAL_BOOLEAN:
            if (value->boolean) {
                printf("True");
            } else {
                printf("False");
            }
            break;


        case CVAL_NUMBER:
            printf("%li", value->num);
            break;

        case CVAL_FLOAT:
            printf("%Lg", value->fnum);
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

        case CVAL_FAULT:
#if SYSTEM_LANG==0
            printf("shtirred: %s", value->err);
#else
            printf("fault: %s", value->err);
#endif
            break;

        case CVAL_SYMBOL:
            printf("%s", value->sym);
            break;

        case CVAL_S_EXPRESSION:
            return cval_expr_print(value, '(', ')');

        case CVAL_Q_EXPRESSION:
            cval_expr_print(value, '{', '}');
            break;

        case CVAL_STRING:
            cval_print_str(value);
            break;

        case CVAL_DICTIONARY:
            hash_table_print(value->ht);
            break;

        case CVAL_NULL:
            printf("NULL");
            break;
    }

    return true;
}
void cval_print_line(cval* value) {
    if (cval_print(value)) {
    putchar('\n'); }
}


