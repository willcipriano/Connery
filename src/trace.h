#ifndef CONNERY_TRACE_H
#define CONNERY_TRACE_H
#include "cval.h"

typedef struct cval cval;

typedef struct trace_entry {
    int position;
    cval* data;
    struct trace_entry* next;
    struct trace_entry* prev;
} trace_entry;

typedef struct trace {
    cval* source;
    struct trace_entry* first;
    struct trace_entry* current;
} trace;

trace* start_trace(char* s);
void record_trace(trace* t, cval* data);
void record_str_trace(trace* t, char* data);
cval* fetch_traceback_data(trace* t, int x);

#endif //CONNERY_TRACE_H

