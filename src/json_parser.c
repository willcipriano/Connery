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

    switch (type) {
        case json_type_object:
            temp_ht = hash_table_create(JSON_OBJECT_HT_INIT_SIZE);

            json_object_object_foreach(json_obj, key, value) {
                hash_table_set(temp_ht, key, parse_object(value));
            }
            return cval_dictionary(temp_ht);

        case json_type_array:
            temp_cval = cval_q_expression();
            temp_size = json_object_array_length(json_obj);
            int cur = 0;

            while (cur < temp_size) {
                cval_add(temp_cval, parse_object(json_object_array_get_idx(json_obj, cur)));
                cur += 1;
            }
            return temp_cval;

        case json_type_string:
            return cval_string((char *) json_object_get_string(json_obj));

        case json_type_int:
            return cval_number(json_object_get_int64(json_obj));

        case json_type_boolean:
            if (json_object_get_boolean(json_obj)) {
                return cval_boolean(true);
            } else {
                return cval_boolean(false);
            }

        case json_type_null:
            return cval_null();

        case json_type_double:
            return cval_float(json_object_get_double(json_obj));

        default:
            return cval_null();
    }
}

cval *parse_json_cval_string(cval* value) {
    return parse_object(json_tokener_parse(value->str));
}

