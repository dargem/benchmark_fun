# benchmarks

some random benchmarks of questionable accuracy but is hopefully interesting
probably doesn't work on 32 bit systems due to hacky stuff!

# Benches!

Seems like standard deviation can be out of wack occasionally.
Not sure why but probably due to rare events like os jitter, core migrations and stuff like that which can skew one sample to take far longer.

# Sorting to help with branch prediction

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

# Random access of char vs uint8_t vs bool vectors

C++ uses a template specialization for bool vectors where it does bit packing of 8 bools in a byte.
This is good for memory but as memory is addressed by the byte this leads to some issues,

- Runtime overhead when reading / writing as bit manipulation is needed to access the specific bit
- Can't take a reference to a bool in a vector
- Accessing a bool in a vector returns a proxy object which is used to access it

Would expect random access to be the same for char and uint8_t vectors, longer for bool vectors.
Benchmarks show this is usually the case, this is a bench of accessing 1000 random indices from a 10000 element array.
~ equal for char and uint_8, ~ 50% longer for bool vectors.

---Summary statistics for bool vector random access--- <br>
Sample mean cycles per test: 1447.27 <br>
Confidence interval: 1441.81-1452.72 <br>
Sample standard deviation: 87.8774 <br>
Tests used: 1000 <br>

---Summary statistics for char vector random access--- <br>
Sample mean cycles per test: 982.49 <br>
Confidence interval: 972.436-992.544 <br>
Sample standard deviation: 162.025 <br>
Tests used: 1000 <br>

---Summary statistics for uint8_t vector random access--- <br>
Sample mean cycles per test: 976.6 <br>
Confidence interval: 974.153-979.047 <br>
Sample standard deviation: 39.4373 <br>
Tests used: 1000 <br>

But interestingly bool vectors can have improved performance in certain memory access patterns.
Increasing the size of the array can lead to bool vectors getting improved speeds.
Accessing 5000 random indexes from a 100000 element array was around equal for all 3.
This is presumably due to the bit packing leading to less cache misses which can be costly.
Using char or uint8_t elements this is ~100000 bytes or 100kb which is > than this cpu's L1 data cache of 32kb.
Using bit packing this will only be ~12500 bytes which can all fit inside the L1 cache easily.
Then increasing the number of iterations over this same large array improves speeds further.
500000 random indexes over the 100000 element array gave these results, where the bitpacking results in good improvements.
Random indexing involves iterating over a vector of size t's which is also interesting,
this would use up far more memory than the actual array of elements here,
since this iteration is sequential its prefetched though.
I assume the cache is managed smartly and since the size_t vector can be predictably prefetched
it doesn't end up wasting a lot of space in the cache.

---Summary statistics for bool vector random access--- <br>

Sample mean cycles per test: 807507 (8.075e+05)<br>
Confidence interval: 787985-827030 <br>
Sample standard deviation: 314599 <br>
Tests used: 1000 <br>

---Summary statistics for char vector random access--- <br>
Sample mean cycles per test: 1.02091e+06 <br>
Confidence interval: 990697-1.05113e+06 <br>
Sample standard deviation: 486933 <br>
Tests used: 1000 <br>

---Summary statistics for uint8_t vector random access--- <br>
Sample mean cycles per test: 1.00104e+06 <br>
Confidence interval: 973945-1.02813e+06 <br>
Sample standard deviation: 436552 <br>
Tests used: 1000 <br>

# plans

- proper statistics with anova, levene test, residual normality checking and other stuff
- prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
- array of structures vs structure of arrays
- simd/vectorisation falls into SOA kinda
- false sharing messing with concurrency
- maybe LTO
- random number generator vs inbuilt
