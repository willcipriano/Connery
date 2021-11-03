#include "mpc.h"
#include "allocator.h"
#include "json_parser.h"
#include <json-c/json.h>
#include "globals.h"
#include "cval.h"
#include "hashtable.h"

cval *parse_object(json_object* json_obj) {
    json_type type = json_object_get_type(json_obj);
    hash_table *temp_ht = NULL;

    int temp_size;
    cval* temp_cval = NULL;
    cval* return_val = cval_null();

    switch (type) {
        case json_type_object:
            temp_ht = hash_table_create(JSON_OBJECT_HT_INIT_SIZE);

            json_object_object_foreach(json_obj, key, value) {
                hash_table_set(temp_ht, key, parse_object(value));
            }
            return_val = cval_dictionary(temp_ht);
            break;

        case json_type_array:
            return_val = cval_q_expression();
            temp_size = json_object_array_length(json_obj);
            int cur = 0;

            while (cur < temp_size) {
                cval_add(return_val, parse_object(json_object_array_get_idx(json_obj, cur)));
                cur += 1;
            }
            break;


        case json_type_string:
            return_val = cval_string((char *) json_object_get_string(json_obj));
            break;

        case json_type_int:
            return_val = cval_number(json_object_get_int64(json_obj));
            break;

        case json_type_boolean:
            if (json_object_get_boolean(json_obj)) {
                return_val = cval_boolean(true);
            } else {
                return_val = cval_boolean(false);
            }
            break;

        case json_type_null:
            return_val = cval_null();
            break;

        case json_type_double:
            return_val = cval_float(json_object_get_double(json_obj));
            break;
    }

    json_object_put(json_obj);
    return return_val;
}

cval *parse_json_cval_string(cval* value) {
    if (value->type == CVAL_STRING) {
        return parse_object(json_tokener_parse(value->str));
    }

    // if response from http, skip grab step
    if (value->type == CVAL_DICTIONARY) {
        cval* body = hash_table_get(value->ht, "body");
        if (body != NULL) {
            if (body->type == CVAL_STRING) {
                return parse_object(json_tokener_parse(body->str));
            }
        }
    }

    return cval_fault("Unable to parse JSON from provided %s", ctype_name(value->type));
}

