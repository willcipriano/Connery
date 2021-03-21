#ifndef CONNERY_TRACE_H
#define CONNERY_TRACE_H
#include <time.h>


typedef struct cval cval;

typedef struct trace_entry {
    int position;
    cval* data;
    struct traceback_entry* next;
    struct traceback_entry* prev;
} trace_entry;

typedef struct trace {
    cval* source;
    int count;
    struct trace_entry* first;
    struct trace_entry* current;
} trace;

#endif //CONNERY_TRACE_H

