#include "cval.h"
#include "trace.h"

trace* start_trace(char* s) {

    trace *tr = malloc(sizeof(trace) * 1);
    trace_entry *te = malloc(sizeof(trace_entry) * 1);

    te->position=0;
    te->data=cval_string("initialization of trace");

    tr->current=te;
    tr->first=te;
    tr->source=cval_string(s);

    return tr;
}

void record_trace(trace* t, cval* data) {
    trace_entry *te = malloc(sizeof(trace_entry) * 1);

    te->position=t->current->position + 1;
    te->prev=t->current;
    te->data=data;

    t->current->next=te;
    t->current=te;
}

void record_str_trace(trace* t, char* data) {
    record_trace(t, cval_string(data));
}

cval* fetch_traceback_data(trace* t, int x) {
    trace_entry* cur_te = t->current;
    cval* traceback = cval_q_expression();
    int y = 0;

    if (x > t->current->position) {
        x = t->current->position;
    }

    while (y <= x) {
        cval_add(traceback, cur_te->data);
        cur_te = t->current->prev;
        y += 1;
    }

    return traceback;
}

