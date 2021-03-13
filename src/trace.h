#ifndef CONNERY_TRACE_H
#define CONNERY_TRACE_H

typedef struct cval cval;

typedef struct trace_entry {
    int position;
    cval data;
    struct traceback_entry* next;
    struct traceback_entry* prev;
} trace_entry;

typedef struct trace {
    cval source;
    int count;
    struct traceback_entry* first;
    struct traceback_entry* current;
} trace;

#endif //CONNERY_TRACE_H

