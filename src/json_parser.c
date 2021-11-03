#include "json_parser.h"
#include <json-c/json.h>
#include "globals.h"
#include "cval.h"
#include "hashtable.h"

cval *parse_object(json_object* json_obj) {
    json_type type = json_object_get_type(json_obj);
    hash_table *ht = NULL;

    switch (type) {
        case json_type_object:
            ht = hash_table_create(JSON_OBJECT_HT_INIT_SIZE);

            json_object_object_foreach(json_obj, key, value) {
                hash_table_set(ht, key, parse_object(value));
            }
            return cval_dictionary(ht);

        case json_type_string:
            return cval_string(json_object_get_string(json_obj));

        case json_type_int:
            return cval_number(json_object_get_int64(json_obj));

        default:
            return cval_null();
    }
}

cval *parse_json_cval_string(cval* value) {
    return parse_object(json_tokener_parse(value->str));
}

