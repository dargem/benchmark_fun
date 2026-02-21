# benchmarks
some random benchmarks of questionable accuracy but is hopefully interesting 
probably doesn't work on 32 bit systems due to hacky stuff!

# benches!
Benchmark is an iteration through a list of random numbers in range a to b.
If that number is larger than (a+b)/2 a number gets incremented.
The compiler is stopped from making this loop branchless,
so the cpu has to do branch predictions which are effectively random.
An incorrect branch prediction requires the core to flush the instruction pipeline.
This leads to a ~15-20 cycle cost on an incorrect prediction. 
Sorting this list before iteration helps the branch predictor,
as for the ~ first half of the list it'll be smaller than the mean (approximately) while its larger than for the second half.
This makes branch prediction very easy, so sorting a list should result in better performance.

---Summary statistics for Branch Prediction Sorted Version--- <br>
Sample mean cycles per test: 2.11696e+07 <br>
Confidence interval: 2.10239e+07-2.13152e+07 <br>
Sample standard deviation: 734015 <br>
Tests used: 100 <br>

---Summary statistics for Branch Prediction Unsorted Version--- <br>
Sample mean cycles per test: 3.44761e+07 <br>
Confidence interval: 3.43189e+07-3.46334e+07 <br>
Sample standard deviation: 792475 <br>
Tests used: 100 <br>



# plans
* proper statistics with anova, levene test, residual normality checking and other stuff
* prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
* array of structures vs structure of arrays
* simd/vectorisation falls into SOA kinda
* false sharing messing with concurrency
* maybe LTO
* random number generator vs inbuilt