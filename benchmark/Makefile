PYTHON ?= python3
BENCH_ARGS ?=

export BENCH_ARGS

WORK = work

.PHONY: all run fixtures generated small medium large lua lua-all compare clean

all: run

run:
	$(PYTHON) bench.py --work $(WORK)

fixtures:
	$(PYTHON) bench.py --work $(WORK) --suite fixtures

generated:
	$(PYTHON) bench.py --work $(WORK) --suite generated

small:
	$(PYTHON) bench.py --work $(WORK) --size small

medium:
	$(PYTHON) bench.py --work $(WORK) --size medium

large:
	$(PYTHON) bench.py --work $(WORK) --size large

lua:
	$(PYTHON) bench.py --work $(WORK) --lua lua

lua-all:
	$(PYTHON) bench.py --work $(WORK) --lua-all

compare:
ifndef BASELINE
	$(error BASELINE is required, e.g. make compare BASELINE=../baseline/lua)
endif
	$(PYTHON) bench.py --work $(WORK) --compare $(BASELINE)

clean:
	rm -rf $(WORK)
