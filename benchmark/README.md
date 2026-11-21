# Benchmarks

This directory contains the benchmark harness for `lua-compiled`. It compares
three execution strategies on the same Lua sources:

- the compiler itself: parse → semantic → IR → optimize → assembly emission
- the generated native binary
- a real Lua interpreter (`lua`, `lua5.4`, `luajit`, etc.)

The harness is a single dependency-free Python 3 script: `bench.py`.

## Quick start

From the repository root:

```bash
make bench
```

Or directly:

```bash
python3 benchmark/bench.py
python3 benchmark/bench.py --size large
python3 benchmark/bench.py --lua luajit
python3 benchmark/bench.py --lua-all
python3 benchmark/bench.py --no-lua
```

From this directory:

```bash
make run
make large
make lua-all
make run BENCH_ARGS="--size small --repeats 30"
```

`bench.py` builds `build/lua` with `make build` if it is missing.

## What is measured

For every source file the report contains:

- `compile min` / `compile med` — compiler wall time, including process startup
- `asm min` — `cc` time to assemble and link the emitted `.s`
- `native min` — wall time of the generated native program
- `src B` / `asm B` — input and emitted assembly sizes
- `IR pre` / `IR post` / `opt %` — IR instruction counts before and after optimization
- `MB/s` — source bytes per second through the compiler
- `<interp> min` — wall time of the same source under a Lua interpreter
- `x vs <interp>` — interpreter time divided by native time

Timings are wall-clock `perf_counter_ns()` around process execution, so process
startup is included. Each benchmark gets warmup runs that are not timed, then
`--repeats` timed runs. The table uses the minimum for noisy process-level
numbers and the median for compiler comparisons.

Small fixture programs are startup-bound; the generated workloads are the
meaningful runtime comparisons.

## Source sets

```bash
python3 benchmark/bench.py --suite fixtures     # files in ../testfiles
python3 benchmark/bench.py --suite generated    # deterministic generated sources
python3 benchmark/bench.py --suite all          # both (default)
```

Generated sizes:

| size | arithmetic stmts | branch iters | outer loop iters | functions | string cases |
| --- | ---: | ---: | ---: | ---: | ---: |
| `small` | 100 | 100 | 20 | 20 | 50 |
| `medium` | 1000 | 1000 | 100 | 100 | 300 |
| `large` | 5000 | 5000 | 300 | 500 | 1000 |

The generated sources are written to `work/generated/` and are valid plain Lua
as well as valid input for this compiler, which is why they can be timed under
both.

## Lua interpreters

By default the first available command is used, in this order:

```
lua, lua5.4, lua5.3, luajit, lua5.2, lua5.1
```

Use `--lua COMMAND` to choose one, repeat it for several, or use `--lua-all`
to benchmark every interpreter found in `PATH`. Use `--no-lua` to skip this
part entirely.

If a source cannot be executed by a selected interpreter, that case is skipped
for that interpreter with a warning on stderr.

## Comparing compiler revisions

Build the old compiler somewhere else, then:

```bash
python3 benchmark/bench.py --compare /path/to/old/lua
```

The comparison table reports median compiler time, minimum native runtime, and
assembly size deltas. Negative percentages are improvements.

## Machine-readable output

```bash
python3 benchmark/bench.py --json work/results.json --csv work/results.csv
```

JSON includes per-run samples, interpreter versions, and full configuration.
CSV contains the summary fields shown in the table.

## Layout

```
benchmark/
  bench.py       # generator, timer, reporter, comparison and JSON/CSV writer
  Makefile       # convenient targets
  README.md      # this file
  work/          # generated sources, .s files, native binaries, results
```
