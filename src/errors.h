#ifndef ERRORS_H
#define ERRORS_H

#include <stdio.h>

#if defined(DEBUG_LUA)
#define D(x) x
#else
#define D(x)
#endif

#define E(x) x
#define MAX_TRACE_STACK_DEPTH 200

typedef enum {
    Undefined,
    CallingBeforeDeclaration,
    FileNotFound,
    NoError
} error_type;

typedef struct
{
    error_type error;
    char *filename;
    long long linenumber;
    int colnumber;
} CallBack;

typedef struct
{
    CallBack *current; /* Current CallBack */
    CallBack *next;    /* Pointer to next CallBack */
    CallBack *last;    /* Last TraceStack in the CallBack */
    CallBack *first;   /* First CallBack in the tracestack */
    int level;         /* TraceStack level = 0 if its empty */
} TraceStack;

void init_trace_stack(TraceStack *tracestack, char *filename);
void trace_stack_insert(TraceStack *tracestack);
CallBack trace_stack_pop(TraceStack *tracestack);
CallBack *generate_callback(error_type error, char *filename,
                            long long linenumber, int colnumber);
int stdout_tracestack();
void terminate();
/* If any panic error occurs then terminate the process of
                     compilation */

extern TraceStack global_tracestack;

#endif // ERRORS_H
