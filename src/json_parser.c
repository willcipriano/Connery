#include "json_parser.h"
#include <json-c/json.h>
#include "globals.h"
#include "cval.h"
#include "hashtable.h"

cval *parse_json_object(json_object* json_obj) {
    json_type type = json_object_get_type(json_obj);
    hash_table *temp_ht = NULL;

    int temp_size;
    cval* return_val = NULL;

    switch (type) {
        case json_type_object:
            temp_ht = hash_table_create(JSON_OBJECT_HT_INIT_SIZE);

            json_object_object_foreach(json_obj, key, value) {
                hash_table_set(temp_ht, key, parse_json_object(value));
            }
            return_val = cval_dictionary(temp_ht);
            break;

        case json_type_array:
            return_val = cval_q_expression();
            temp_size = json_object_array_length(json_obj);
            int cur = 0;

            while (cur < temp_size) {
                cval_add(return_val, parse_json_object(json_object_array_get_idx(json_obj, cur)));
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

        default:
#if SYSTEM_LANG == 0
            return cval_fault("unable to parshe jshon object with content: '%s'", json_object_get_string(json_obj));
#else
        return cval_fault("unable to parse json object with content: '%s'", json_object_get_string(json_obj));
#endif

    }
    // free object
    json_object_put(json_obj);

    return return_val;
}

json_object *parse_cval_dict(cenv* env, cval *object);

json_object *parse_cval_object(cenv* env, cval *object) {
    json_object* return_object = NULL;
    int cur = 0;

    switch(object->type) {
        case CVAL_DICTIONARY:
            return parse_cval_dict(env, object);

        case CVAL_Q_EXPRESSION:
        case CVAL_S_EXPRESSION:
            return_object = json_object_new_array();

            while (cur < object->count) {
                json_object_array_add(return_object, parse_cval_object(env, object->cell[cur]));
                cur += 1;
            }
            return return_object;

        case CVAL_NUMBER:
            return json_object_new_int64(object->num);

        case CVAL_BOOLEAN:
            if (object->boolean) {
                return json_object_new_boolean(true);
            } else {
                return json_object_new_boolean(false);
            }

        case CVAL_STRING:
            return json_object_new_string(object->str);

        case CVAL_SYMBOL:
            return parse_cval_object(env, cval_evaluate(env, object));

        default:
            return NULL;
    }

}

json_object *parse_cval_dict(cenv* env, cval *object) {
    json_object * return_object = json_object_new_object();
    cval ** temp_keys = hash_table_dump_keys(object->ht);

    cval * curObj = NULL;

    for (int i = 0; i < object->ht->items; ++i) {
        curObj = hash_table_get(object->ht, temp_keys[i]->str);
        json_object_object_add(return_object, temp_keys[i]->str, parse_cval_object(env, curObj));
    }

    return return_object;
}

cval *parse_json_cval(cenv* env, cval* value) {
    // if it's a string, check to see if it's a json string
    json_object* obj;

    if (value->type == CVAL_STRING) {
        obj = json_tokener_parse(value->str);
        if (obj != NULL) {
            return parse_json_object(obj);
        }
        free(obj);
    }

    // if response from the http builtin, parse the body
    if (value->class == CVAL_CLASS_HTTP && value->type == CVAL_DICTIONARY) {
        cval* body = hash_table_get(value->ht, "body");
        if (body != NULL) {
            obj = json_tokener_parse(body->str);
            if (obj != NULL) {
                return parse_json_object(obj);
            }
            free(obj);
        }
        return cval_fault("unable to parshe provided jshon.");
    }

    cval* mode = safe_cenv_get(env, "__JSON_OUTPUT_MODE__");
    obj = parse_cval_object(env, value);

    // if mode is configured, use that
    if (mode != NULL) {
        char * modeStr = mode->str;
        if (strstr(modeStr, "pretty")) {
            return cval_string((char *) json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
        }

        if (strstr(modeStr, "compact")) {
            return cval_string((char *) json_object_to_json_string_ext(obj, JSON_C_TO_STRING_NOZERO));
        }

        if (strstr(modeStr, "basic")) {
            return cval_string((char *) json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN));
        }
    }

    // otherwise, convert the object into a json string with the default output mode
    return cval_string((char *) json_object_to_json_string_ext(parse_cval_object(env, value), JSON_STRING_DEFAULT_OUTPUT_MODE));
}

