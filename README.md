# benchmarks
some random benchmarks for fun
<br>
CPU specs: Ryzen 7 260, 8 core
* L1i 32 kb per core
* L1d 32 kb per core
* L2 1 mb per core
* l3 16 mb shared

probably doesn't work on 32 bit systems due to hacky stuff!

# plans
* benchrunner which can output some proper statistics for comparison
* prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
* array of structures vs structure of arrays
* simd/vectorisation falls into SOA kinda
* sorting a list to help with branch prediction
* false sharing messing with concurrency
* maybe LTO
* random number generator vs inbuilt