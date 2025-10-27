#include "errors.h"
#include <stdlib.h>

void init_trace_stack(TraceStack *tracestack) {
  tracestack->callframes = NULL;
  tracestack->symbols = NULL;
  tracestack->calltop = 0;
  tracestack->symboltop = 0;
}

void tracestack_push_frame(TraceStack *tracestack, const char *functioncall,
                           const char *filename, size_t line_number) {

  if (tracestack->calltop >= MAX_TRACE_STACK_DEPTH) {
    fprintf(stderr,
            "Fatal Error: StackOverflow! max call depth (%d) reached.\n",
            MAX_TRACE_STACK_DEPTH);
    exit(1);
  }

  CallFrame *newframe = malloc(sizeof(CallFrame));
  if (newframe == NULL) {
    fprintf(stderr,
            "Fatal Error: New function call memory allocation failed!\n");
    exit(1);
  }
  newframe->functioncall = functioncall;
  newframe->line_number = line_number;
  newframe->filename = filename;

  newframe->back = tracestack->callframes;
  tracestack->callframes = newframe;
  tracestack->calltop++;
}

CallFrame tracestack_peek_frame(TraceStack *tracestack) {
  if (tracestack->calltop == 0) {
    fprintf(stderr, "Fatal Error: StackUnderFlow! tired to remove frame from "
                    "an empty stack");
    exit(1);
  }
  return *tracestack->callframes;
}

void tracestack_pop_frame(TraceStack *tracestack) {
  if (tracestack->calltop == 0) {
    fprintf(stderr, "Fatal Error: StackUnderFlow! tired to remove frame from "
                    "an empty stack");
    exit(1);
  }
  CallFrame *newframe = tracestack->callframes;
  tracestack->callframes = tracestack->callframes->back;
  tracestack->calltop--;
  free(newframe);
}

void tracestack_push_symbol(TraceStack *tracestack, const char *symbolname,
                            const char *type, const char *filename,
                            size_t line_number) {

  if (tracestack->symboltop >= MAX_TRACE_STACK_DEPTH) {
    fprintf(stderr,
            "Fatal Error: StackOverflow! max symbol depth (%d) reached.\n",
            MAX_TRACE_STACK_DEPTH);
    exit(1);
  }

  Symbol *newframe = malloc(sizeof(Symbol));
  if (newframe == NULL) {
    fprintf(stderr,
            "Fatal Error: New function call memory allocation failed!\n");
    exit(1);
  }
  newframe->symbolname = symbolname;
  newframe->line_number = line_number;
  newframe->filename = filename;
  newframe->type = type;

  newframe->back = tracestack->symbols;
  tracestack->symbols = newframe;
  tracestack->symboltop++;
}

Symbol tracestack_peek_symbol(TraceStack *tracestack) {
  if (tracestack->symboltop == 0) {
    fprintf(stderr, "Fatal Error: StackUnderFlow! tired to remove frame from "
                    "an empty stack");
    exit(1);
  }
  return *tracestack->symbols;
}

void tracestack_pop_symbol(TraceStack *tracestack) {
  if (tracestack->symboltop == 0) {
    fprintf(stderr, "Fatal Error: StackUnderFlow! tired to remove frame from "
                    "an empty stack");
    exit(1);
  }
  Symbol *newframe = tracestack->symbols;
  tracestack->symbols = tracestack->symbols->back;
  tracestack->symboltop--;
  free(newframe);
}
