#ifndef ERRORS_H
#define ERRORS_H

#define DEBUG_LUA
#include <stdio.h>

#if defined(DEBUG_LUA)
#define D(x) x
#else
#define D(x)
#endif

#define E(x) x
#define MAX_TRACE_STACK_DEPTH 200

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

extern TraceStack global_trace;

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
