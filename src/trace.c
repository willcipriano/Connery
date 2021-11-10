#include "cval.h"
#include "trace.h"

trace* start_trace(char* s) {
    trace *tr = malloc(sizeof(trace) * 1);
    trace_entry *te = malloc(sizeof(trace_entry) * 1);

    te->position=0;
    te->data=cval_string("-- initialization of trace --");

    tr->current=te;
    tr->first=te;
    tr->source=cval_string(s);
    return tr;
}

void record_trace(trace* t, cval* data) {
    trace_entry *te = calloc(1, sizeof(trace_entry));

    te->position=t->current->position + 1;
    te->prev=t->current;
    te->data=cval_copy(data);

    t->current->next=te;
    t->current=te;
}
