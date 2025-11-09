#include "errors.h"
#include <stdlib.h>

TraceStack global_tracestack;

void init_trace_stack(TraceStack *tracestack) {
  tracestack = malloc(sizeof(TraceStack));
  tracestack->level = 0;
}
