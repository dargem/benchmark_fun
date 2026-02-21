# benchmarks
some random benchmarks for fun
<br>
CPU specs: Ryzen 7 260, 8 core
* L1i 32 kb per core
* L1d 32 kb per core
* L2 1 mb per core
* l3 16 mb shared

probably doesn't work on 32 bit systems due to hacky stuff!

# benches!
Benchmark is an iteration through a list of random numbers in range a to b
If that number is larger than the mid range a number gets incremented
The compiler is stopped from making this loop branchless
Sorting this list before iteration helps the branch predictor 
as its effectively a lot of smaller than val, followed by a lot of larger than val

---Summary statistics for Branch Prediction Sorted Version---
Sample mean cycles per test: 2.11696e+07
Confidence interval: 2.10239e+07-2.13152e+07
Sample standard deviation: 734015
Tests used: 100

---Summary statistics for Branch Prediction Unsorted Version---
Sample mean cycles per test: 3.44761e+07
Confidence interval: 3.43189e+07-3.46334e+07
Sample standard deviation: 792475
Tests used: 100

# plans
* benchrunner which can output some proper statistics for comparison
* prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
* array of structures vs structure of arrays
* simd/vectorisation falls into SOA kinda
* false sharing messing with concurrency
* maybe LTO
* random number generator vs inbuilt