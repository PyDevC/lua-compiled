#include "errors.h"
#include <stdlib.h>

/**
 * Setup the Location to the Errors or Debug
 *
 * E(fprintf(stderr, "Syntax Error: At line: %d\n%s\n%s\n", token.linenumber,
 * token.literal, ErrorMsg));
 *
 * D(fprintf(stdout, "DEBUG: At line: 49 -> Value of char c in get_next_char:
 * %c\n", c));
 *
 **/

TraceStack global_tracestack;

void init_trace_stack(TraceStack *tracestack, char *filename)
{
    tracestack = malloc(sizeof(TraceStack));
    CallBack *first = generate_callback(0, filename, 0, 0);

    tracestack->current = first;
    tracestack->first = first;
    tracestack->last = first;
    tracestack->next = NULL;
    tracestack->level = 0;
}

CallBack *generate_callback(error_type error, char *filename,
                            long long linenumber, int colnumber)
{
    CallBack *first = malloc(sizeof(CallBack));
    first->linenumber = linenumber;
    first->colnumber = colnumber;
    first->error = error;
    first->filename = filename;
    return first;
}

int stdout_tracestack()
{
    /*Running through whole tracestack */
    return 0;
}

void terminate()
{
    int error = stdout_tracestack();
    exit(error);
}
