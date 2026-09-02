# MIPS Cache Timing Simulator

This project extends a five-stage MIPS timing simulator with separate
instruction and data caches. It includes benchmarks for spatial locality,
temporal locality, block size, associativity, and cache capacity.

## Cache logic

The default configuration follows the lab specification:

- Instruction cache: 8 KiB, four-way set associative, 32-byte blocks
- Data cache: 64 KiB, eight-way set associative, 32-byte blocks
- Replacement policy: least recently used (LRU)
- Miss penalty: 50 cycles
- Initially empty instruction and data caches

For each access, the simulator determines the memory block, set, and tag:

```c
block_number = address / block_size;
set = block_number % number_of_sets;
tag = block_number / number_of_sets;
```

It searches every way in the selected set for a valid matching tag. A hit
updates the LRU state. A miss stalls the relevant pipeline stage for 50 cycles,
then inserts the block into an invalid way or replaces the LRU block.

The data-cache geometry can be changed at compile time using the defaults in
`src/pipe.h`:

```c
DATA_CACHE_SIZE_BYTES
DATA_CACHE_BLOCK_SIZE_BYTES
DATA_CACHE_ASSOCIATIVITY
```

The number of sets is derived automatically from these values. The instruction
cache remains fixed during the data-cache parameter experiments.

## Building and running

Build and run the default configuration:

```bash
make
make run
```

Run all added cache benchmarks:

```bash
python3 run.py \
  inputs/cache/stride1.x \
  inputs/cache/stride_long.x \
  inputs/cache/temporal_frequent.x \
  inputs/cache/temporal_late.x \
  inputs/cache/associativity.x \
  inputs/cache/capacity.x
```

To test another data-cache configuration, override one parameter while keeping
the others at their defaults. For example:

```bash
gcc -g -O2 -DDATA_CACHE_ASSOCIATIVITY=4 src/*.c -o sim
python3 run.py inputs/cache/associativity.x
```

Restore the default configuration after a sweep:

```bash
gcc -g -O2 src/*.c -o sim
```

## Test results

Detailed baseline comparisons, cycles, IPC values, and reproduction commands
are stored in the following files:

- [Spatial-locality results](stride_test_results.txt)
- [Temporal-locality results](temporal_test_results.txt)
- [Block-size comparison](block_size_compare_results.txt)
- [Associativity comparison](associativity_compare_results.txt)
- [Cache-size comparison](cache_size_compare_results.txt)
- [Complete simulator test results](test_results.txt)

All added benchmarks produce architectural register contents identical to the
baseline simulator. Cycle counts and IPC differ because the baseline does not
model the added cache-miss delays.

## Main conclusions

- Larger blocks help sequential accesses by exploiting spatial locality, but
  do not help patterns that move to a different block on every access.
- Frequent reuse produces temporal-locality hits while reuse after enough
  same-set conflicts causes eviction under LRU.
- Greater associativity prevents conflict misses when several active blocks
  map to the same set.
- Greater capacity prevents repeated scans from thrashing when the complete
  working set fits in the cache.
- Increasing a cache parameter further provides no additional IPC improvement
  once the benchmark's relevant locality requirement is satisfied.
