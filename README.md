# lua-compiled

A small Lua-subset **native compiler** written the way Casey Muratori writes C:
one translation unit, no per-token allocation, no cascading headers, and flat
arrays of data with indices instead of pointer webs.

There is no VM, bytecode interpreter, or tree-walking evaluator in the final
path. Source goes through four passes and leaves as GAS Intel-syntax x86-64
assembly:

```
source -> AST -> semantic/type check -> typed IR -> optimize -> .s file -> native executable
```

## Style: where it comes from

This is not an invented "clean code" style. It is taken from actual
Casey Muratori source:

- `handmade_platform.h` — `i8`/`u32`/`b32`/`f64`, `internal`,
  `global_variable`, `ArrayCount`, `Assert`, `InvalidCodePath`, `Minimum`.
- `handmade.h` — `memory_arena`, `PushStruct`, `PushArray`, `PushSize`,
  `ZeroStruct`, `BeginTemporaryMemory`/`EndTemporaryMemory`.
- `win32_handmade.cpp` — the platform file `#include`s the implementation,
  so there is exactly one compiler invocation.
- `computerenhance.com/perfaware/sim86` — table-driven parsing
  (`Rules[TK_COUNT]` here plays the role of the instruction table),
  `Result = {}` initialization and `return(Result)`.

The lexer in `src/lexer.c` is the original starting point: `lexer_cursor`,
`GlobalKeywords`, `LookupKeyword`, `ScanNextToken`, `token_to_string`. Names,
strings and comments now go through `CopySliceToCString`, which pushes into
the compile arena instead of calling `malloc` for every token. Number scanning
accumulates the value directly, so the old fixed `NumBuffer[64]` is gone too.

## What changed from the old version

- Deleted the half-finished arena, hash map, symbol table, errors, old
  semantic/IR modules, tree-walking evaluator, and per-file header maze.
  `src/lua.c` includes all implementation `.c` files directly.
- Token text is arena-owned: no `strdup`, no `free_token`, and no fixed-size
  token buffers.
- The Pratt parser uses one rule table and proper left associativity
  (`a - b - c` is now `(a - b) - c`). Fixed the broken `elseif` chain, added
  `return`, `break` and `do ... end`.
- AST nodes live in two dense arrays sized once from the input length: no
  `realloc` during parsing, and `ASTIndex` instead of pointers.
- Added strict static semantic analysis. Every name resolves to a flat symbol
  slot, every expression gets one type, function arity and return types are
  checked, scopes are explicit, and undefined names or mixed-type uses are
  errors before codegen. Logical operators and `not` require booleans; builtins
  can only appear as direct calls.
- `Type_Unknown` is an inference variable: the first concrete use fixes it,
  and later mismatches report a semantic error. Lua's dynamic `nil`/boolean/
  number/string coercion is intentionally not part of this subset.
- Added a typed three-address IR. Registers are virtual; jumps use instruction
  indices; function bodies are emitted inline in one instruction array with
  patch stacks and per-register types.
- Added an IR optimizer. It tracks virtual-register definitions, folds constant
  expressions and constant branches, removes copy chains into stable
  registers, prunes unreachable blocks and dead pure instructions, redirects
  jump chains, and compacts the instruction array while fixing function entry
  labels and branch targets.
- Added the AMD64 backend. It prints `.intel_syntax noprefix` assembly with a
  `main` wrapper that calls `lua_main`. Each runtime value is a 16-byte typed
  frame slot: numbers occupy the first word as a double, booleans/integers/
  function pointers as an integer, and strings as pointer + length. User
  functions take a hidden argument-block pointer in `rdi` and return scalars
  in `rax`/`rdx` or `xmm0`.
- `print` lowers to libc `printf`; `type` lowers to a static type-name string.
- Parse and semantic errors are ordinary messages in one side-channel log. Each
  token and AST node carries one byte offset; `file:line:column` is computed only
  when a message is printed. Impossible compiler states still hit `Assert`;
  expected failures do not.
- Numeric `for` loops keep their counter in a virtual register and only spill it
  to the symbol slot when the body can observe it. An integer-valued empty loop
  lowers to a GPR counter loop instead of floating-point frame-slot traffic.
- The backend fuses compare/branch pairs, keeps dependent arithmetic chains in
  `xmm0`, and packs independent same-operation arithmetic into SSE
  `addpd`/`subpd`/`mulpd`/`divpd` when the IR exposes a pair.
- The lexer is SIMD-friendly: identifier and whitespace runs are scanned eight
  bytes at a time with 64-bit SWAR masks, and keyword lookup compares a padded
  64-bit load instead of calling `strncmp` per keyword.
- The compiler's own hot paths are deliberately slim. Tokens and AST nodes store
  one source offset instead of line and column, assembly is formatted into one
  arena buffer and written with a single `fwrite`, and the emitter precomputes
  register-use, last-definition, function-entry and jump-target tables once
  instead of rescanning the IR for every peephole decision.

## Error architecture

The diagnostic model follows Ryan Fleury's "The Easiest Way To Handle Errors Is
To Not Have Them" while keeping Casey Muratori's flat C style.

- **Errors are cases, not exceptions.** There is no `Result<T, E>` union and no
  `HadError` flag duplicated in every pass. A pass returns its normal data and
  appends messages to the shared `message_log`.
- **AND instead of OR.** The parser produces an AST *and* messages; the semantic
  pass produces symbol/type data *and* messages. The log is a linked list of
  `message` structs pushed from its own arena, so it can carry a source offset,
  origin, and text without a new type per failure mode.
- **Zero is initialization.** `message_list`, `cmdline_params`, and
  `message_log` are valid when zeroed. An empty log loops zero times. A failed
  parse returns `ASTNone`, which every later pass treats as an empty tree.
- **Nil structs make reads safe.** `NilSource`, `NilSymbol`, and `NilFunction`
  are readable defaults with empty slices and zeroed fields. Lookup results can
  be consumed without a null check on the read path; they are never written.
- **Fail early, in shallow frames.** `main` uses a fixed stack buffer for the
  diagnostic arena, parses the command line, sizes the input file, allocates the
  one compile arena, and opens the output file before entering code generation.
  Expected I/O failures are logged at that boundary, not deep in a pass.
- **One reporting path.** Every exit from `main` goes through the `done` label:
  close the output file, print the message log once, free the compile arena, and
  return `1` only if the log contains errors.
- **Assertions stay assertions.** `Assert`/`InvalidCodePath` are reserved for
  impossible internal invariants. They are not used for user input, bad files,
  type errors, or missing output permissions.

`src/messages.c` is the whole diagnostic subsystem. `src/lua.c` owns the log,
and the parser/semantic passes only append to it.

## Compiler speed

The error refactor briefly made the compiler slower, so the second pass was to
remove work from the compiler itself without changing the generated assembly.
`cmp` against the pre-refactor `.s` file is byte-for-byte identical for every
fixture, including `testfiles/big.lua`.

Measured with `cc -O3`, `-s /dev/null`, interleaved runs on this machine:

| target | before median | now median | change |
| --- | ---: | ---: | ---: |
| `testfiles/big.lua` (80 runs) | 5.269 ms | 4.740 ms | ~10% faster |
| all fixtures (25 rounds) | 0.633 ms | 0.644 ms | flat within noise |

On `big.lua`, the `opt` pass dropped from 0.581 ms to 0.422 ms after the third
optimization pass was removed as a measured no-op, and `emit` dropped from
2.399 ms to 2.012 ms after buffered output and precomputed IR tables replaced
per-line `vfprintf` calls and repeated whole-program scans.

The small-fixture number is dominated by process startup; the large-file number
is the useful signal for compiler throughput.

## Build

```bash
make            # build compiler, emit .s for every fixture, assemble, run
make build      # just build build/lua
make debug      # unoptimized with -DDEBUG_LUA=1
make clean
```

## Benchmark

See `benchmark/README.md`. The harness measures compile time, emitted assembly
size, IR optimization counts, native runtime, and runtime under a real Lua
interpreter.

```bash
make bench
python3 benchmark/bench.py --lua-all
```

## Run

```bash
./build/lua testfiles/expressions.lua          # writes testfiles/expressions.s
cc testfiles/expressions.s -o build/expressions
./build/expressions

./build/lua -s build/smoke.s testfiles/smoke.lua
./build/lua -p testfiles/function.lua          # parse + type check only
./build/lua -d testfiles/smoke.lua             # dump the AST
./build/lua -i testfiles/smoke.lua             # dump the typed IR
./build/lua -v testfiles/smoke.lua
```

CLI flags:

- `-s PATH` — override the default assembly path (`<input>.s`).
- `-i` — dump the optimized typed IR before assembly.
- `-d` — dump the parsed AST.
- `-p` — parse and type check only; emit no assembly.
- `-v` — show per-pass timing and counts on stderr.

`make test` emits `build/<name>.s`, assembles it with `$(CC)`, and runs the
resulting native program for every fixture. The fixtures keep their syntax
coverage but use strict, statically typed programs (defined globals, numeric
boolean-arithmetic replacements, a terminating `repeat`, a local `require`
shim, and complete call arguments).

The supported syntax is in `Grammar/grammar_current`.
