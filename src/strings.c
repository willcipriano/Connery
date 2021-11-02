#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "cval.h"
#include "globals.h"

#define CASSERT(args, cond, fmt, ...) \
if (!(cond)) {\
    cval* err = cval_fault(fmt, ##__VA_ARGS__); \
    cval_delete(args); \
    return err;}

#if SYSTEM_LANG==0
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

#if SYSTEM_LANG==0
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


char *int_to_string(long integer) {
    char *str = malloc(11);
    sprintf(str, "%ld", integer);
    return str;
}

// https://creativeandcritical.net/str-replace-c
char *repl_str(const char *str, const char *from, const char *to) {

    /* Adjust each of the below values to suit your needs. */

    /* Increment positions cache size initially by this number. */
    size_t cache_sz_inc = 16;
    /* Thereafter, each time capacity needs to be increased,
     * multiply the increment by this factor. */
    const size_t cache_sz_inc_factor = 3;
    /* But never increment capacity by more than this number. */
    const size_t cache_sz_inc_max = 1048576;

    char *pret, *ret = NULL;
    const char *pstr2, *pstr = str;
    size_t i, count = 0;

    uintptr_t *pos_cache_tmp, *pos_cache = NULL;

    size_t cache_sz = 0;
    size_t cpylen, orglen, retlen, tolen, fromlen = strlen(from);

    /* Find all matches and cache their positions. */
    while ((pstr2 = strstr(pstr, from)) != NULL) {
        count++;

        /* Increase the cache size when necessary. */
        if (cache_sz < count) {
            cache_sz += cache_sz_inc;
            pos_cache_tmp = realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
            if (pos_cache_tmp == NULL) {
                goto end_repl_str;
            } else pos_cache = pos_cache_tmp;
            cache_sz_inc *= cache_sz_inc_factor;
            if (cache_sz_inc > cache_sz_inc_max) {
                cache_sz_inc = cache_sz_inc_max;
            }
        }

        pos_cache[count-1] = pstr2 - str;
        pstr = pstr2 + fromlen;
    }

    orglen = pstr - str + strlen(pstr);

    /* Allocate memory for the post-replacement string. */
    if (count > 0) {
        tolen = strlen(to);
        retlen = orglen + (tolen - fromlen) * count;
    } else	retlen = orglen;
    ret = malloc(retlen + 1);
    if (ret == NULL) {
        goto end_repl_str;
    }

    if (count == 0) {
        /* If no matches, then just duplicate the string. */
        strcpy(ret, str);
    } else {
        /* Otherwise, duplicate the string whilst performing
         * the replacements using the position cache. */
        pret = ret;
        memcpy(pret, str, pos_cache[0]);
        pret += pos_cache[0];
        for (i = 0; i < count; i++) {
            memcpy(pret, to, tolen);
            pret += tolen;
            pstr = str + pos_cache[i] + fromlen;
            cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - fromlen;
            memcpy(pret, pstr, cpylen);
            pret += cpylen;
        }
        ret[retlen] = '\0';
    }

    end_repl_str:
    /* Free the cache and return the post-replacement string,
     * which will be NULL in the event of an error. */
    free(pos_cache);
    return ret;
}

void replaceFirstChar(char * str, char oldChar, char newChar)
{
    int i=0;

    while(str[i] != '\0')
    {
        if(str[i] == oldChar)
        {
            str[i] = newChar;
            break;
        }
        i++;
    }
}

// https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c/11394336#11394336
char* concat(int count, ...)
{
    va_list ap;
    int i;

    // Find required length to store merged string
    int len = 1; // room for NULL
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
        len += strlen(va_arg(ap, char*));
    va_end(ap);

    // Allocate memory to concat strings
    char *merged = calloc(sizeof(char),len);
    int null_pos = 0;

    // Actually concatenate strings
    va_start(ap, count);
    for(i=0 ; i<count ; i++)
    {
        char *s = va_arg(ap, char*);
        strcpy(merged+null_pos, s);
        null_pos += strlen(s);
    }
    va_end(ap);

    return merged;
}

cval *builtin_string_replace_first_char(cenv *e, cval *a) {
    CASSERT_TYPE("replace_first_char", a, 0, CVAL_STRING);
    CASSERT_TYPE("replace_first_char", a, 1, CVAL_STRING);
    CASSERT_TYPE("replace_first_char", a, 2, CVAL_STRING);
    CASSERT_NUM("replace_first_char", a, 3);

    if (strlen(a->cell[1]->str) == 1) {
        if (strlen(a->cell[2]->str) == 1) {
            char* new_str = a->cell[0]->str;
            replaceFirstChar(new_str, a->cell[1]->str[0], a->cell[2]->str[0]);

            cval* new_cval = cval_string(new_str);
            cval_delete(a);

            return new_cval;
        }
    }

#if SYSTEM_LANG==0
    return cval_fault("replace first char takesh three shtringsh, and the shecond and third musht be only one character long!");
#else
    return cval_fault("replace first char takes three strings, and the second and third must be only one character long!");
#endif
}

cval *builtin_string_replace_all(cenv *e, cval *a) {
    CASSERT_TYPE("replace_all", a, 0, CVAL_STRING);
    CASSERT_TYPE("replace_all", a, 1, CVAL_STRING);
    CASSERT_TYPE("replace_all", a, 2, CVAL_STRING);
    CASSERT_NUM("replace_all", a, 3);

    cval *new_cval = cval_string(repl_str(a->cell[0]->str, a->cell[1]->str, a->cell[2]->str));
    cval_delete(a);

    return new_cval;
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
        return cval_fault("Index %i out of bounds", split_index);
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

cval *builtin_concat(cenv *e, cval *a) {
//    todo: rewrite this
    CASSERT_TYPE("concat", a, 0, CVAL_STRING);
    CASSERT_TYPE("concat", a, 1, CVAL_STRING);

    cval *new_cval;

    switch (a->count) {
        case 2:
            new_cval = cval_string(concat(2, a->cell[0]->str, a->cell[1]->str));
            break;

        case 3:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            new_cval = cval_string(concat(3, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str));
            break;

        case 4:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            new_cval = cval_string(concat(4, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str));
            break;

        case 5:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            new_cval = cval_string(concat(5, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str));
            break;

        case 6:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            CASSERT_TYPE("concat", a, 5, CVAL_STRING);
            new_cval = cval_string(concat(6, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str, a->cell[5]->str));
            break;

        case 7:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            CASSERT_TYPE("concat", a, 5, CVAL_STRING);
            CASSERT_TYPE("concat", a, 6, CVAL_STRING);
            new_cval = cval_string(concat(7, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str, a->cell[5]->str, a->cell[6]->str));
            break;

        case 8:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            CASSERT_TYPE("concat", a, 5, CVAL_STRING);
            CASSERT_TYPE("concat", a, 6, CVAL_STRING);
            CASSERT_TYPE("concat", a, 7, CVAL_STRING);
            new_cval = cval_string(concat(8, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str, a->cell[5]->str, a->cell[6]->str, a->cell[7]->str));
            break;

        case 9:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            CASSERT_TYPE("concat", a, 5, CVAL_STRING);
            CASSERT_TYPE("concat", a, 6, CVAL_STRING);
            CASSERT_TYPE("concat", a, 7, CVAL_STRING);
            CASSERT_TYPE("concat", a, 8, CVAL_STRING);
            new_cval = cval_string(concat(9, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str, a->cell[5]->str, a->cell[6]->str, a->cell[7]->str, a->cell[8]->str));
            break;

        case 10:
            CASSERT_TYPE("concat", a, 2, CVAL_STRING);
            CASSERT_TYPE("concat", a, 3, CVAL_STRING);
            CASSERT_TYPE("concat", a, 4, CVAL_STRING);
            CASSERT_TYPE("concat", a, 5, CVAL_STRING);
            CASSERT_TYPE("concat", a, 6, CVAL_STRING);
            CASSERT_TYPE("concat", a, 7, CVAL_STRING);
            CASSERT_TYPE("concat", a, 8, CVAL_STRING);
            CASSERT_TYPE("concat", a, 9, CVAL_STRING);
            new_cval = cval_string(concat(10, a->cell[0]->str, a->cell[1]->str, a->cell[2]->str, a->cell[3]->str, a->cell[4]->str, a->cell[5]->str, a->cell[6]->str, a->cell[7]->str, a->cell[8]->str, a->cell[9]->str));
            break;

        default:
            new_cval = cval_fault("concat accepts between 2 and 10 strings.");
    }

    cval_delete(a);
    return new_cval;
}
