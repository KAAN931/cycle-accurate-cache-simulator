# MIPS Pipeline Simulator with Memory Hierarchy

A cycle-accurate five-stage MIPS pipeline timing simulator, built following the
Lab 1 and Lab 2 specifications from the Computer Architecture course
(227-2210-00L) by Prof. Onur Mutlu.

## What was provided

A skeleton timing simulator (`pipe.c`, `pipe.h`) that models a five-stage
in-order MIPS pipeline with correct architectural behavior (stalling, bypassing,
flushing). A baseline simulator (`basesim`) is included for verifying
architectural correctness.

## Lab 1 — L1 Caches

Added separate instruction and data caches to the pipeline:

- **Instruction cache:** 8 KiB, 4-way set-associative, 32-byte blocks
- **Data cache:** 64 KiB, 8-way set-associative, 32-byte blocks
- LRU replacement, initially empty
- Fixed 50-cycle miss penalty

Custom benchmarks were written to test spatial locality, temporal locality,
block size, associativity, and cache capacity effects on IPC.

## Lab 2 — L2 Cache and DRAM (this submission)

Replaced the fixed 50-cycle L1 miss penalty with a full memory hierarchy. On an
L1 miss, the L2 cache is now probed in the same cycle, and misses go to a
DRAM-based main memory through a memory controller.

### Unified L2 Cache

- 256 KiB, 16-way set-associative, 32-byte blocks (512 sets)
- Set index: address bits [13:5]
- 16 MSHRs (miss-status holding registers) to track outstanding misses
- True LRU replacement; new blocks inserted at MRU position
- **L2 hit latency:** 15 cycles
- **L2 miss:** request sent to memory controller after 5 cycles; fill returns
  to L2 after another 5 cycles once DRAM serves the request

When both the fetch and memory stages hit different blocks in the same L2 set
in one cycle, the memory-stage block is promoted to MRU-1 and the fetch-stage
block to MRU.

### DRAM Main Memory

- Single channel, single rank, 8 banks
- 64K rows per bank, 8 KB per row
- Bank index: address bits [7:5]; row index: bits [31:16]
- Row buffers initially closed; open-row policy

**DRAM commands and timing:**

| Command     | Bus usage        | Bank busy |
|-------------|------------------|-----------|
| ACTIVATE    | cmd/addr 4 cyc   | 100 cyc   |
| READ/WRITE  | cmd/addr 4 cyc   | 100 cyc   |
| PRECHARGE   | cmd/addr 4 cyc   | 100 cyc   |
| Data xfer   | data bus 50 cyc  | —         |

**Row-buffer scenarios:**

| Scenario            | Command sequence                  |
|---------------------|-----------------------------------|
| Row-buffer hit      | READ/WRITE                        |
| Row-buffer miss     | ACTIVATE, READ/WRITE              |
| Row-buffer conflict | PRECHARGE, ACTIVATE, READ/WRITE   |

### Memory Controller (FR-FCFS)

The memory controller scans the request queue each cycle and schedules
requests using FR-FCFS priority:

1. Row-buffer hits over non-hits
2. Earlier arrivals over later arrivals
3. Memory-stage requests over fetch-stage requests

A request is schedulable only when all its commands can be issued without
conflicts on the command/address bus, data bus, and target bank.

## Building and Running

```bash
make            # build the simulator
make run        # run all tests and compare against baseline
```

Run a specific test:

```bash
make run INPUT=inputs/inst/addiu.x
```

Build the baseline simulator (provided reference):

```bash
make basesim
```

## Project Structure

```
src/
  pipe.c    - pipeline simulator with L1/L2 cache and DRAM logic
  pipe.h    - data structures (caches, MSHRs, DRAM banks, requests)
  mips.h    - MIPS instruction definitions
  shell.c   - interactive simulator shell
  shell.h   - shell header
inputs/       - test programs (.x binaries)
run.py        - automated test runner
Makefile      - build system
```

## Test Results

Baseline comparison outputs (architectural correctness verified; cycle counts
differ due to the modeled memory hierarchy):

- [Spatial-locality results](stride_test_results.txt)
- [Temporal-locality results](temporal_test_results.txt)
- [Block-size comparison](block_size_compare_results.txt)
- [Associativity comparison](associativity_compare_results.txt)
- [Cache-size comparison](cache_size_compare_results.txt)
- [Full test results](test_results.txt)
