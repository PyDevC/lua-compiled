PYTHON ?= python3
BENCH_ARGS ?=

WORK = work

.PHONY: all run fixtures generated small medium large lua lua-all compare clean

all: run

run:
	$(PYTHON) bench.py --work $(WORK) $(BENCH_ARGS)

fixtures:
	$(PYTHON) bench.py --work $(WORK) --suite fixtures $(BENCH_ARGS)

generated:
	$(PYTHON) bench.py --work $(WORK) --suite generated $(BENCH_ARGS)

small:
	$(PYTHON) bench.py --work $(WORK) --size small $(BENCH_ARGS)

medium:
	$(PYTHON) bench.py --work $(WORK) --size medium $(BENCH_ARGS)

large:
	$(PYTHON) bench.py --work $(WORK) --size large $(BENCH_ARGS)

lua:
	$(PYTHON) bench.py --work $(WORK) --lua lua $(BENCH_ARGS)

lua-all:
	$(PYTHON) bench.py --work $(WORK) --lua-all $(BENCH_ARGS)

compare:
ifndef BASELINE
	$(error BASELINE is required, e.g. make compare BASELINE=../baseline/lua)
endif
	$(PYTHON) bench.py --work $(WORK) --compare $(BASELINE) $(BENCH_ARGS)

clean:
	rm -rf $(WORK)
