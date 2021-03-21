#include "cval.h"
#include "trace.h"

trace_entry *start_trace() {

    trace *tr = malloc(sizeof(trace) * 1);
    trace_entry *te = malloc(sizeof(trace_entry) * 1);

    tr->count=0;
    tr->current=te;
    tr->first=te;
    tr->source=cval_string("initialization");


    te->position=0;
    te->data=cval_string("initialization of trace")

}

