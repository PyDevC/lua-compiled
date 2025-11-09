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
typedef struct TraceStack TraceStack;

struct CallFrame {
  const char *functioncall;
  const char *filename;
  size_t line_number;
  TraceStack *next;
};

struct Symbol {
  const char *symbolname;
  const char *filename;
  size_t line_number;
  TraceStack *next;
};

struct TraceStack {
  enum { CALL, SYM } type;
  size_t level;

  union {
    CallFrame *callframes;
    Symbol *symbols;
  } frame;
};

extern TraceStack global_tracestack;

void init_trace_stack(TraceStack *tracestack);

#endif // ERRORS_H
