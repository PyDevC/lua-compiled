#include "errors.h"
#include <stdlib.h>

/*
 * Setup the Location to the Errors or Debug
 *
 * E(fprintf(stderr, "Syntax Error: At line: %d\n%s\n%s\n", token.linenumber,
 * token.literal, ErrorMsg));
 *
 * D(fprintf(stdout, "DEBUG: At line: 49 -> Value of char c in get_next_char:
 * %c\n", c));
 *
 */

TraceStack global_tracestack;

void init_trace_stack(TraceStack *tracestack)
{
    tracestack = malloc(sizeof(TraceStack));
    tracestack->level = 0;
}
