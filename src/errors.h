#ifndef ERRORS_H
#define ERRORS_H

#if defined(DEBUG_LUA)
#define D(X)                                                                   \
  if (p->debug) {                                                              \
    x;                                                                         \
  }

#else
#define D()
#endif

#define E(x)                                                                   \
  {                                                                            \
    x;                                                                         \
  }

#define MAX_TRACE_STACK_DEPTH 200

#include <stdio.h>

typedef struct CallFrame CallFrame;
typedef struct Symbol Symbol;

struct CallFrame {
  const char *functioncall;
  const char *filename;
  size_t line_number;
  CallFrame *back;
};

struct Symbol {
  const char *symbolname;
  const char *type;
  const char *filename;
  size_t line_number;
  Symbol *back;
};

typedef struct {
  CallFrame *callframes;
  Symbol *symbols;
  size_t calltop;
  size_t symboltop;
} TraceStack;

void init_trace_stack(TraceStack *tracestack);

void tracestack_push_frame(TraceStack *tracestack, const char *functioncall,
                           const char *filename, size_t line_number);
CallFrame tracestack_peek_frame(TraceStack *tracestack);
void tracestack_pop_frame(TraceStack *tracestack);

void tracestack_push_symbol(TraceStack *tracestack, const char *symbolname,
                            const char *type, const char *filename,
                            size_t line_number);
Symbol tracestack_peek_symbol(TraceStack *tracestack);
void tracestack_pop_symbol(TraceStack *tracestack);

#endif // ERRORS_H
