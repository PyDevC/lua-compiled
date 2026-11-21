#!/usr/bin/env python3
"""End-to-end benchmarks for the lua-compiled native compiler.

The harness measures three things for every source file:

  1. compiler time  - parse + semantic + IR + optimization + assembly emission
  2. assembler time - cc turning the emitted .s file into a native binary
  3. native runtime - the generated program itself

When a Lua interpreter is available it also runs the same source file and
reports interpreter time plus native speedup. No third-party Python packages
are required.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

ROOT = Path(__file__).resolve().parent.parent
BENCH_ROOT = Path(__file__).resolve().parent
DEFAULT_COMPILER = ROOT / "build" / "lua"
DEFAULT_WORK = BENCH_ROOT / "work"

RE_IR = re.compile(r"\bir:\s+(\d+) instructions")
RE_OPT = re.compile(r"\bopt:\s+(\d+)\s+->\s+(\d+) instructions")

LUA_CANDIDATES = ["lua", "lua5.4", "lua5.3", "luajit", "lua5.2", "lua5.1"]

SCALES = {
    "small": {
        "arithmetic": 100,
        "branches": 100,
        "loops": 20,
        "functions": 20,
        "strings": 50,
    },
    "medium": {
        "arithmetic": 1000,
        "branches": 1000,
        "loops": 100,
        "functions": 100,
        "strings": 300,
    },
    "large": {
        "arithmetic": 5000,
        "branches": 5000,
        "loops": 300,
        "functions": 500,
        "strings": 1000,
    },
}

GENERATOR_ORDER = ["arithmetic", "branches", "loops", "functions", "strings"]


@dataclass
class Case:
    name: str
    path: Path
    kind: str


@dataclass
class Interpreter:
    command: str
    label: str
    version: str


@dataclass
class Measurement:
    case: str
    kind: str
    source: str
    source_bytes: int
    source_lines: int
    asm_bytes: int
    ir_pre: Optional[int]
    ir_post: Optional[int]
    compiler: str
    cc: str
    compile_min_ms: float
    compile_median_ms: float
    compile_mean_ms: float
    compile_stdev_ms: float
    assemble_min_ms: float
    assemble_median_ms: float
    native_min_ms: float
    native_median_ms: float
    native_mean_ms: float
    native_stdev_ms: float
    compile_samples_ms: List[float]
    assemble_samples_ms: List[float]
    native_samples_ms: List[float]


@dataclass
class InterpreterMeasurement:
    case: str
    interpreter: str
    interpreter_command: str
    interpreter_version: str
    min_ms: float
    median_ms: float
    mean_ms: float
    stdev_ms: float
    samples_ms: List[float]


def command_list(command: Sequence[object]) -> List[str]:
    return [str(part) for part in command]


def run_command(
    command: Sequence[object],
    *,
    cwd: Optional[Path] = None,
    stdout: int = subprocess.DEVNULL,
    stderr: int = subprocess.DEVNULL,
) -> subprocess.CompletedProcess:
    return subprocess.run(
        command_list(command),
        cwd=str(cwd) if cwd else None,
        stdout=stdout,
        stderr=stderr,
        check=False,
    )


def check_command(
    command: Sequence[object],
    *,
    cwd: Optional[Path] = None,
    stdout: int = subprocess.DEVNULL,
    stderr: int = subprocess.DEVNULL,
) -> subprocess.CompletedProcess:
    result = run_command(command, cwd=cwd, stdout=stdout, stderr=stderr)
    if result.returncode != 0:
        raise RuntimeError(
            "command failed (%d): %s" % (result.returncode, " ".join(command_list(command)))
        )
    return result


def time_command(
    command: Sequence[object],
    *,
    repeats: int,
    warmup: int,
    cwd: Optional[Path] = None,
) -> List[float]:
    for _ in range(warmup):
        check_command(command, cwd=cwd)

    samples = []
    for _ in range(repeats):
        start = time.perf_counter_ns()
        check_command(command, cwd=cwd)
        end = time.perf_counter_ns()
        samples.append((end - start) / 1_000_000.0)
    return samples


def summarize(samples: Sequence[float]) -> Tuple[float, float, float, float]:
    return (
        min(samples),
        statistics.median(samples),
        statistics.mean(samples),
        statistics.stdev(samples) if len(samples) > 1 else 0.0,
    )


def source_size(path: Path) -> Tuple[int, int]:
    data = path.read_bytes()
    lines = data.count(b"\n")
    if lines == 0 or not data.endswith(b"\n"):
        lines += 1
    return len(data), lines


def parse_counts(text: str) -> Tuple[Optional[int], Optional[int]]:
    ir_match = RE_IR.search(text)
    opt_match = RE_OPT.search(text)
    if opt_match:
        return int(opt_match.group(1)), int(opt_match.group(2))
    if ir_match:
        value = int(ir_match.group(1))
        return value, value
    return None, None


def safe_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", name).strip("_")


def gen_arithmetic(count: int) -> str:
    lines = [
        "-- generated arithmetic stress",
        "acc = 0",
        "local k = 3.0",
    ]
    for index in range(1, count + 1):
        lines.append(
            "acc = acc + (%d.0 + %d.0 * k - %d.0 / 2.0)"
            % (index, index + 1, index + 2)
        )
    lines.append("print(acc)")
    return "\n".join(lines) + "\n"


def gen_branches(count: int) -> str:
    return "\n".join(
        [
            "-- generated branch stress",
            "local limit = %d" % count,
            "hits = 0",
            "for i = 1, limit do",
            "  if i < 10 then",
            "    hits = hits + 1",
            "  elseif i > limit - 10 then",
            "    hits = hits + 2",
            "  else",
            "    hits = hits + 3",
            "  end",
            "  if i == 5 or i == 10 then",
            "    hits = hits + 1",
            "  end",
            "end",
            "print(hits)",
        ]
    ) + "\n"


def gen_loops(count: int) -> str:
    inner = max(4, count // 10)
    return "\n".join(
        [
            "-- generated loop stress",
            "local inner = %d" % inner,
            "acc = 0",
            "for i = 1, %d do" % count,
            "  local j = 0",
            "  while j < inner do",
            "    acc = acc + i + j",
            "    j = j + 1",
            "    if j > inner - 2 then",
            "      break",
            "    end",
            "  end",
            "end",
            "print(acc)",
        ]
    ) + "\n"


def gen_functions(count: int) -> str:
    lines = [
        "-- generated function stress",
        "function bench_fib(n)",
        "  if n <= 1 then",
        "    return n",
        "  end",
        "  return bench_fib(n - 1) + bench_fib(n - 2)",
        "end",
        "",
    ]
    for index in range(count):
        lines.append("function bench_f%d(x)" % index)
        lines.append("  return x + %d" % index)
        lines.append("end")
    lines.append("")
    lines.append("acc = 0")
    for index in range(count):
        lines.append("acc = acc + bench_f%d(%d)" % (index, index))
    lines.append("print(acc, bench_fib(12))")
    return "\n".join(lines) + "\n"


def gen_strings(count: int) -> str:
    value = "benchmark-string-value"
    lines = [
        "-- generated string stress",
        's = "%s"' % value,
        "hits = 0",
        'if s == "%s" then' % value,
        "  hits = hits + 1",
        "end",
        "if #s == %d then" % len(value),
        "  hits = hits + 1",
        "end",
    ]
    for index in range(count):
        lines.append('if s == "bench-case-%d" then' % index)
        lines.append("  hits = hits + 1")
        lines.append("end")
        lines.append("if #s > %d then" % index)
        lines.append("  hits = hits + 1")
        lines.append("end")
    lines.append("print(hits, type(s))")
    return "\n".join(lines) + "\n"


GENERATORS = {
    "arithmetic": gen_arithmetic,
    "branches": gen_branches,
    "loops": gen_loops,
    "functions": gen_functions,
    "strings": gen_strings,
}


def generate_cases(work: Path, size: str) -> List[Case]:
    generated_dir = work / "generated"
    generated_dir.mkdir(parents=True, exist_ok=True)

    cases = []
    for name in GENERATOR_ORDER:
        path = generated_dir / ("%s.lua" % name)
        path.write_text(GENERATORS[name](SCALES[size][name]), encoding="utf-8")
        cases.append(Case("gen/%s" % name, path, "generated"))
    return cases


def fixture_cases() -> List[Case]:
    fixture_dir = ROOT / "testfiles"
    paths = sorted(fixture_dir.glob("*.lua"))
    return [Case("fixture/%s" % path.stem, path, "fixture") for path in paths]


def resolve_input_path(value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path.resolve()
    cwd_candidate = (Path.cwd() / path).resolve()
    if cwd_candidate.exists():
        return cwd_candidate
    return (ROOT / path).resolve()


def ensure_compiler(compiler: Path) -> None:
    if compiler.is_file():
        return
    if compiler.resolve() == DEFAULT_COMPILER.resolve():
        print("building compiler with make...", file=sys.stderr)
        check_command(["make", "-C", str(ROOT), "build"])
        return
    raise RuntimeError("compiler not found: %s" % compiler)


def interpreter_version(command: str) -> str:
    try:
        result = run_command(
            [command, "-v"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
        text = result.stdout.decode("utf-8", errors="replace")
        for line in text.splitlines():
            if line.strip():
                return line.strip()
    except OSError:
        pass
    return "unknown"


def make_interpreter(spec: str) -> Interpreter:
    command = None
    path = Path(spec)
    if path.is_file() and os.access(str(path), os.X_OK):
        command = str(path.resolve())
    else:
        found = shutil.which(spec)
        if found:
            command = found
    if not command:
        raise RuntimeError("Lua interpreter not found: %s" % spec)

    label = Path(command).name
    if label.endswith(".exe"):
        label = label[:-4]
    return Interpreter(command=command, label=label, version=interpreter_version(command))


def select_interpreters(args: argparse.Namespace) -> List[Interpreter]:
    if args.no_lua:
        return []

    if args.lua:
        specs = list(args.lua)
    elif args.lua_all:
        specs = []
        for candidate in LUA_CANDIDATES:
            found = shutil.which(candidate)
            if found and found not in specs:
                specs.append(found)
        if not specs:
            print("bench: no Lua interpreter found in PATH; skipping", file=sys.stderr)
            return []
    else:
        specs = []
        for candidate in LUA_CANDIDATES:
            found = shutil.which(candidate)
            if found:
                specs.append(found)
                break
        if not specs:
            print("bench: no Lua interpreter found in PATH; skipping", file=sys.stderr)
            return []

    interpreters = []
    seen = set()
    for spec in specs:
        interpreter = make_interpreter(spec)
        if interpreter.command in seen:
            continue
        seen.add(interpreter.command)
        interpreters.append(interpreter)
    return interpreters


def measure_case(
    case: Case,
    *,
    compiler: Path,
    cc: str,
    work: Path,
    tag: str,
    repeats: int,
    warmup: int,
) -> Measurement:
    stem = "%s__%s" % (tag, safe_name(case.name))
    asm_path = work / (stem + ".s")
    binary_path = work / (stem + ".bin")

    validate = check_command(
        [compiler, "-v", "-s", asm_path, case.path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    ir_pre, ir_post = parse_counts(validate.stderr.decode("utf-8", errors="replace"))
    asm_bytes = asm_path.stat().st_size
    source_bytes, source_lines = source_size(case.path)

    compile_samples = time_command(
        [compiler, "-s", os.devnull, case.path],
        repeats=repeats,
        warmup=warmup,
    )
    assemble_samples = time_command(
        [cc, asm_path, "-o", binary_path],
        repeats=repeats,
        warmup=warmup,
    )
    native_samples = time_command(
        [binary_path],
        repeats=repeats,
        warmup=warmup,
    )

    cmin, cmed, cmean, cstdev = summarize(compile_samples)
    amin, amed, _, _ = summarize(assemble_samples)
    nmin, nmed, nmean, nstdev = summarize(native_samples)

    return Measurement(
        case=case.name,
        kind=case.kind,
        source=str(case.path),
        source_bytes=source_bytes,
        source_lines=source_lines,
        asm_bytes=asm_bytes,
        ir_pre=ir_pre,
        ir_post=ir_post,
        compiler=str(compiler),
        cc=cc,
        compile_min_ms=cmin,
        compile_median_ms=cmed,
        compile_mean_ms=cmean,
        compile_stdev_ms=cstdev,
        assemble_min_ms=amin,
        assemble_median_ms=amed,
        native_min_ms=nmin,
        native_median_ms=nmed,
        native_mean_ms=nmean,
        native_stdev_ms=nstdev,
        compile_samples_ms=compile_samples,
        assemble_samples_ms=assemble_samples,
        native_samples_ms=native_samples,
    )


def measure_all(
    cases: Sequence[Case],
    *,
    compiler: Path,
    cc: str,
    work: Path,
    tag: str,
    repeats: int,
    warmup: int,
) -> List[Measurement]:
    results = []
    for case in cases:
        print("measuring %s..." % case.name, file=sys.stderr)
        results.append(
            measure_case(
                case,
                compiler=compiler,
                cc=cc,
                work=work,
                tag=tag,
                repeats=repeats,
                warmup=warmup,
            )
        )
    return results


def measure_interpreter(
    cases: Sequence[Case],
    *,
    interpreter: Interpreter,
    repeats: int,
    warmup: int,
) -> List[InterpreterMeasurement]:
    results = []
    for case in cases:
        print(
            "measuring %s on %s..." % (case.name, interpreter.label),
            file=sys.stderr,
        )
        try:
            samples = time_command(
                [interpreter.command, case.path],
                repeats=repeats,
                warmup=warmup,
            )
        except RuntimeError as error:
            print("bench: skipping %s: %s" % (case.name, error), file=sys.stderr)
            continue

        imin, imed, imean, istdev = summarize(samples)
        results.append(
            InterpreterMeasurement(
                case=case.name,
                interpreter=interpreter.label,
                interpreter_command=interpreter.command,
                interpreter_version=interpreter.version,
                min_ms=imin,
                median_ms=imed,
                mean_ms=imean,
                stdev_ms=istdev,
                samples_ms=samples,
            )
        )
    return results


def optimization_percent(measurement: Measurement) -> Optional[float]:
    if not measurement.ir_pre or measurement.ir_post is None:
        return None
    return (1.0 - measurement.ir_post / measurement.ir_pre) * 100.0


def compile_throughput(measurement: Measurement) -> float:
    if measurement.compile_min_ms <= 0:
        return 0.0
    return measurement.source_bytes / (measurement.compile_min_ms / 1000.0) / 1e6


def speedup(native_ms: float, interpreter_ms: float) -> Optional[float]:
    if native_ms <= 0:
        return None
    return interpreter_ms / native_ms


def format_optional_number(value: Optional[float], digits: int = 1) -> str:
    if value is None:
        return "-"
    return ("%%.%df" % digits) % value


def format_table(headers: Sequence[str], rows: Sequence[Sequence[str]]) -> str:
    widths = [len(header) for header in headers]
    for row in rows:
        for index, cell in enumerate(row):
            widths[index] = max(widths[index], len(cell))
    lines = ["| " + " | ".join(h.ljust(widths[i]) for i, h in enumerate(headers)) + " |"]
    lines.append("| " + " | ".join("-" * width for width in widths) + " |")
    for row in rows:
        lines.append("| " + " | ".join(cell.ljust(widths[i]) for i, cell in enumerate(row)) + " |")
    return "\n".join(lines)


def print_report(
    results: Sequence[Measurement],
    interpreter_results: Optional[Dict[str, List[InterpreterMeasurement]]] = None,
) -> None:
    interpreter_results = interpreter_results or {}
    interpreter_maps = {
        label: {measurement.case: measurement for measurement in measurements}
        for label, measurements in interpreter_results.items()
    }

    headers = [
        "case",
        "src B",
        "asm B",
        "IR pre",
        "IR post",
        "opt %",
        "compile min",
        "compile med",
        "asm min",
        "native min",
        "MB/s",
    ]
    for label in interpreter_results:
        headers.append("%s min" % label)
        headers.append("x vs %s" % label)

    rows = []
    for result in results:
        row = [
            result.case,
            str(result.source_bytes),
            str(result.asm_bytes),
            str(result.ir_pre) if result.ir_pre is not None else "-",
            str(result.ir_post) if result.ir_post is not None else "-",
            format_optional_number(optimization_percent(result)),
            "%.3f" % result.compile_min_ms,
            "%.3f" % result.compile_median_ms,
            "%.3f" % result.assemble_min_ms,
            "%.3f" % result.native_min_ms,
            "%.2f" % compile_throughput(result),
        ]
        for label, by_case in interpreter_maps.items():
            measurement = by_case.get(result.case)
            if measurement:
                row.append("%.3f" % measurement.min_ms)
                row.append(
                    format_optional_number(
                        speedup(result.native_min_ms, measurement.min_ms), 2
                    )
                )
            else:
                row.append("-")
                row.append("-")
        rows.append(row)

    print()
    print(format_table(headers, rows))


def percentage_delta(candidate: float, baseline: float) -> str:
    if baseline == 0:
        return "-"
    return "%+.1f%%" % ((candidate / baseline - 1.0) * 100.0)


def print_comparison(baseline: Sequence[Measurement], candidate: Sequence[Measurement]) -> None:
    baseline_by_name = {result.case: result for result in baseline}
    candidate_by_name = {result.case: result for result in candidate}
    names = [result.case for result in baseline if result.case in candidate_by_name]
    if not names:
        print("no overlapping benchmark cases to compare", file=sys.stderr)
        return

    headers = [
        "case",
        "base compile med",
        "new compile med",
        "delta",
        "base native min",
        "new native min",
        "delta",
        "base asm B",
        "new asm B",
        "delta",
    ]
    rows = []
    for name in names:
        old = baseline_by_name[name]
        new = candidate_by_name[name]
        rows.append(
            [
                name,
                "%.3f" % old.compile_median_ms,
                "%.3f" % new.compile_median_ms,
                percentage_delta(new.compile_median_ms, old.compile_median_ms),
                "%.3f" % old.native_min_ms,
                "%.3f" % new.native_min_ms,
                percentage_delta(new.native_min_ms, old.native_min_ms),
                str(old.asm_bytes),
                str(new.asm_bytes),
                percentage_delta(float(new.asm_bytes), float(old.asm_bytes)),
            ]
        )
    print()
    print("comparison (negative delta is faster/smaller)")
    print(format_table(headers, rows))


def write_json(
    path: Path,
    config: dict,
    results: Sequence[Measurement],
    interpreters: Sequence[Interpreter],
    interpreter_results: Dict[str, List[InterpreterMeasurement]],
) -> None:
    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "config": config,
        "interpreters": [asdict(interpreter) for interpreter in interpreters],
        "results": [asdict(result) for result in results],
        "interpreter_results": {
            label: [asdict(measurement) for measurement in measurements]
            for label, measurements in interpreter_results.items()
        },
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def write_csv(
    path: Path,
    results: Sequence[Measurement],
    interpreter_results: Dict[str, List[InterpreterMeasurement]],
) -> None:
    fields = [
        "case",
        "kind",
        "source",
        "source_bytes",
        "source_lines",
        "asm_bytes",
        "ir_pre",
        "ir_post",
        "compiler",
        "cc",
        "compile_min_ms",
        "compile_median_ms",
        "compile_mean_ms",
        "compile_stdev_ms",
        "assemble_min_ms",
        "assemble_median_ms",
        "native_min_ms",
        "native_median_ms",
        "native_mean_ms",
        "native_stdev_ms",
    ]

    interpreter_maps = {
        label: {measurement.case: measurement for measurement in measurements}
        for label, measurements in interpreter_results.items()
    }
    for label in interpreter_results:
        prefix = safe_name(label)
        fields.extend(
            [
                "%s_min_ms" % prefix,
                "%s_median_ms" % prefix,
                "%s_speedup" % prefix,
            ]
        )

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        for result in results:
            row = asdict(result)
            for label, by_case in interpreter_maps.items():
                prefix = safe_name(label)
                measurement = by_case.get(result.case)
                if measurement:
                    row["%s_min_ms" % prefix] = measurement.min_ms
                    row["%s_median_ms" % prefix] = measurement.median_ms
                    row["%s_speedup" % prefix] = speedup(
                        result.native_min_ms, measurement.min_ms
                    )
                else:
                    row["%s_min_ms" % prefix] = ""
                    row["%s_median_ms" % prefix] = ""
                    row["%s_speedup" % prefix] = ""
            writer.writerow({field: row.get(field, "") for field in fields})


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark the lua-compiled compiler, native output, and Lua interpreters."
    )
    parser.add_argument(
        "--compiler",
        default=str(DEFAULT_COMPILER),
        help="compiler binary to benchmark (default: %(default)s)",
    )
    parser.add_argument(
        "--compare",
        help="baseline compiler binary to compare against the candidate",
    )
    parser.add_argument(
        "--suite",
        choices=("all", "fixtures", "generated"),
        default="all",
        help="which source files to benchmark (default: %(default)s)",
    )
    parser.add_argument(
        "--size",
        choices=tuple(SCALES.keys()),
        default="medium",
        help="generated workload size (default: %(default)s)",
    )
    parser.add_argument(
        "--repeats",
        type=int,
        default=15,
        help="timed runs per benchmark (default: %(default)s)",
    )
    parser.add_argument(
        "--warmup",
        type=int,
        default=3,
        help="untimed warmup runs per benchmark (default: %(default)s)",
    )
    parser.add_argument(
        "--cc",
        default=os.environ.get("CC", "cc"),
        help="C compiler used to assemble emitted .s files (default: %(default)s)",
    )
    parser.add_argument(
        "--work",
        default=str(DEFAULT_WORK),
        help="scratch/output directory (default: %(default)s)",
    )
    parser.add_argument(
        "--lua",
        action="append",
        metavar="COMMAND",
        help="Lua interpreter to benchmark; repeat for multiple interpreters",
    )
    parser.add_argument(
        "--lua-all",
        action="store_true",
        help="benchmark every Lua interpreter found in PATH",
    )
    parser.add_argument(
        "--no-lua",
        action="store_true",
        help="do not benchmark Lua interpreters",
    )
    parser.add_argument("--json", help="also write full results to this JSON file")
    parser.add_argument("--csv", help="also write summary results to this CSV file")
    args = parser.parse_args(argv)
    if args.repeats < 1:
        parser.error("--repeats must be at least 1")
    if args.warmup < 0:
        parser.error("--warmup cannot be negative")
    if args.no_lua and (args.lua or args.lua_all):
        parser.error("--no-lua cannot be combined with --lua or --lua-all")
    return args


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    compiler = resolve_input_path(args.compiler)
    baseline = resolve_input_path(args.compare) if args.compare else None
    work = Path(args.work).resolve()
    work.mkdir(parents=True, exist_ok=True)

    try:
        ensure_compiler(compiler)
        if baseline:
            ensure_compiler(baseline)

        cases = []
        if args.suite in ("all", "fixtures"):
            cases.extend(fixture_cases())
        if args.suite in ("all", "generated"):
            cases.extend(generate_cases(work, args.size))
        if not cases:
            raise RuntimeError("no benchmark cases selected")

        interpreters = select_interpreters(args)
        config = {
            "compiler": str(compiler),
            "baseline": str(baseline) if baseline else None,
            "suite": args.suite,
            "size": args.size,
            "repeats": args.repeats,
            "warmup": args.warmup,
            "cc": args.cc,
            "work": str(work),
            "lua_interpreters": [asdict(interpreter) for interpreter in interpreters],
        }

        baseline_results = None
        if baseline:
            baseline_results = measure_all(
                cases,
                compiler=baseline,
                cc=args.cc,
                work=work,
                tag="base",
                repeats=args.repeats,
                warmup=args.warmup,
            )

        results = measure_all(
            cases,
            compiler=compiler,
            cc=args.cc,
            work=work,
            tag="candidate",
            repeats=args.repeats,
            warmup=args.warmup,
        )

        interpreter_results = {}
        for interpreter in interpreters:
            interpreter_results[interpreter.label] = measure_interpreter(
                cases,
                interpreter=interpreter,
                repeats=args.repeats,
                warmup=args.warmup,
            )

        print_report(results, interpreter_results)
        if baseline_results:
            print_comparison(baseline_results, results)
        if args.json:
            write_json(Path(args.json), config, results, interpreters, interpreter_results)
            print("json: %s" % args.json)
        if args.csv:
            write_csv(Path(args.csv), results, interpreter_results)
            print("csv: %s" % args.csv)
        return 0
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print("bench: %s" % error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
