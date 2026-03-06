# benchmarks

some random benchmarks of questionable accuracy but is hopefully interesting
probably doesn't work on 32 bit systems due to hacky stuff!

# Benches!

Seems like standard deviation can be out of wack occasionally.
Not sure why but probably due to rare events like os jitter, core migrations and stuff like that which can skew one sample to take far longer.

# Structure of Arrays (SOA) vs Array of Structures (AOS) (SIMD test)

Benchmark is iterating over an effective list of entities.
An entity consists of the values: attack, defense, health, x, y and z.
In an iteration it just simply adds x, y and z values by 1.
An array of structure is just a vector of Entity instances, but a better way to do it can be having a structure of arrays.
Rather the collection itself is a class, with vectors for each attack, defense, health and etc.
This can allow vectorization of the code where the CPU performs a single operation on multiple pieces of data in parallel,
also known as simd (single instruction multiple data).
This can lead to massive performance increases.
Cache locality is also arguably better if you only need to iterate over say the coordinates of an entity,
with AOS it would load the whole entity into the cache.
If that entity is large thats obviously inefficient and it might be pushing out actually useful stuff.
Iterating over a float and a long double it would be expected that smaller data types will benefit from SIMD more
Note sizes are implementation defined though, while they are 4 and 16 bytes on my machine respectively,
a long double is only guaranteed to be as long as a double for example.

```
---Summary statistics for Array of Structures Iteration over uint8_t---
Sample mean cycles per test: 44358.8
Confidence interval: 44089.3-44628.3
Sample standard deviation: 3067.37
Tests used: 500

---Summary statistics for Structure of Arrays iteration over uint8_t---
Sample mean cycles per test: 4470.62
Confidence interval: 4354.22-4587.03
Sample standard deviation: 1324.79
Tests used: 500
```

Unbelievable 10x speed increases due to SIMD, SOA is way faster here when operating on the 1 byte unsigned integers.
A more realistic example using floats seeing as these are coordinates.

```
---Summary statistics for Array of Structures Iteration over float---
Sample mean cycles per test: 91756.3
Confidence interval: 87572-95940.6
Sample standard deviation: 47621.7
Tests used: 500

---Summary statistics for Structure of Arrays iteration over float---
Sample mean cycles per test: 31608.8
Confidence interval: 29563.1-33654.5
Sample standard deviation: 23282
Tests used: 500
```

Still ~3x speed increase which is incredible on the 4 byte floats.

```
---Summary statistics for Array of Structures Iteration over double---
Sample mean cycles per test: 91581.5
Confidence interval: 89847.1-93315.9
Sample standard deviation: 19739.5
Tests used: 500

---Summary statistics for Structure of Arrays iteration over double---
Sample mean cycles per test: 54189.8
Confidence interval: 53253.1-55126.5
Sample standard deviation: 10660.7
Tests used: 500
```

Bit under 2x speed increases which is still great for the 8 byte doubles.
Pattern here is fairly obvious, smaller data types take greater advantage of SIMD.
SIMD performs operations on multiple pieces of data in the CPU's register at once,
smaller data types lets the cpu pack more elements in the same register and operate on more data at the same time.

```
---Summary statistics for Array of Structures Iteration over long double---
Sample mean cycles per test: 4.58413e+06
Confidence interval: 4.48494e+06-4.68332e+06
Sample standard deviation: 1.12893e+06
Tests used: 500

---Summary statistics for Structure of Arrays iteration over long double---
Sample mean cycles per test: 4.3403e+06
Confidence interval: 4.27143e+06-4.40917e+06
Sample standard deviation: 783833
Tests used: 500
```

Performance gains almost completely disappear with the 16 byte long double.
SOA is still statistically significantly faster (p < 0.05) but its a very minor speed difference.
The difference could honestly just be due to better cache locality.
The cpu this was run on is based on the zen 4 architecture which has support for AVX-512 instruction set (512 bit aka 64 byte simd instructions),
though through a trick double pumping them through 256 bit registers.
Regardless even using AVX2 instructions for 256 bit registers you would expect to fit 2 long doubles (128 each),
but there's clearly not a sizeable enough performance increase.
On x86-64 linux, long double is 80 bit extended precision stored in 16 bytes, 10 data and 6 padding
(seems like a lot of wasted space but guess it needs to be 8 byte aligned).
AVX2 and AVX512 have no support for 80 bit floats, just vectorizing 32 bit and 64 bit floats.
So the compiler uses scalar instructions and the very minimal performance increases are just due to the better cache locality which is interesting.

<br>

Obviously trying to vectorize code makes it far less readable and hard to maintain + understand,
but up to a 10x performance increase is a must have for some performance critical systems like computer graphics.

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

```
---Summary statistics for Branch Prediction Sorted Version---
Sample mean cycles per test: 1.21501e+06
Confidence interval: 1.19809e+06-1.23193e+06
Sample standard deviation: 148924
Tests used: 300

---Summary statistics for Branch Prediction Unsorted Version---
Sample mean cycles per test: 2.17935e+06
Confidence interval: 2.15972e+06-2.19897e+06
Sample standard deviation: 172743
Tests used: 300
```

As seen decent ~80% performance increase by a seemingly strange optimization for the hardware.

# Random access of char vs uint8_t vs bool vectors

C++ uses a template specialization for bool vectors where it does bit packing of 8 bools in a byte.
This is good for memory but as memory is addressed by the byte this leads to some issues:

- Runtime overhead when reading / writing as bit manipulation is needed to access the specific bit
- Can't take a reference to a bool in a vector
- Accessing a bool in a vector returns a proxy object which is used to access it

Would expect random access to be the same for char and uint8_t vectors, longer for bool vectors.
Benchmarks show this is usually the case, this is a bench of accessing 1000 random indices from a 10000 element array.
~ equal for char and uint_8, ~ 50% longer for bool vectors.

```
---Summary statistics for bool vector random access---
Sample mean cycles per test: 1447.27
Confidence interval: 1441.81-1452.72
Sample standard deviation: 87.8774
Tests used: 1000 <br>

---Summary statistics for char vector random access---
Sample mean cycles per test: 982.49
Confidence interval: 972.436-992.544
Sample standard deviation: 162.025
Tests used: 1000

---Summary statistics for uint8_t vector random access---
Sample mean cycles per test: 976.6
Confidence interval: 974.153-979.047
Sample standard deviation: 39.4373
Tests used: 1000
```

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

```
---Summary statistics for bool vector random access---
Sample mean cycles per test: 807507 (8.075e+05)
Confidence interval: 787985-827030
Sample standard deviation: 314599
Tests used: 1000

---Summary statistics for char vector random access---
Sample mean cycles per test: 1.02091e+06
Confidence interval: 990697-1.05113e+06
Sample standard deviation: 486933
Tests used: 1000

---Summary statistics for uint8_t vector random access---
Sample mean cycles per test: 1.00104e+06
Confidence interval: 973945-1.02813e+06
Sample standard deviation: 436552
Tests used: 1000
```

# XOROSHIRO 128+ vs Inbuilt Mersenne Twister

Considerably better results from the xoroshiro 128++ rng and the inbuilt mersenne twister rng.
Was expecting xoroshiro128+ to be substantially faster and it is, 3x faster number generation is very welcome.
Xoroshiro also has better statistical properties.

```
---Summary statistics for Xoroshiro128+ RNG---
Sample mean cycles per test: 3.09121e+06
Confidence interval: 3.08428e+06-3.09813e+06
Sample standard deviation: 60965.1
Tests used: 300

---Summary statistics for Mersenne twister RNG---
Sample mean cycles per test: 8.83133e+06
Confidence interval: 8.75627e+06-8.90639e+06
Sample standard deviation: 660624
Tests used: 300
```

# LIKELY / UNLIKELY Attributes

C++ 20 introduces the [[likely]] and [[unlikely]] attributes which are "hints" to inform the compiler if a path of execution is more or less likely than another.

```
if constexpr (A == Attribute::UNLIKELY) {
    if (number > SIZE_NEEDED_FOR_SUCCESS) [[unlikely]] {  // lie 95% is unlikely
        successes += 1;
    } else if (number == SIZE_NEEDED_FOR_SUCCESS) [[likely]] {  // lie ~0% is likely
        equalities += 1;
    }
}
```

This benchmarks 3 scenarios like above, the randomly generated number has a ~95% chance to be > size_needed_for_success.
For the number to be equal this was exceedingly rare (1/100000) so lazily evaluation this after the first if should improve performance.
This uses the [[unlikely]] attribute and [[likely]] attribute to lie to the compiler.
If it trusts me this would theoretically tank performance as it would evaluate the unlikely condition first thinking that path of execution was "more likely".
This is obviously just a hint to the compiler, definitely compiler specific and the compiler could just completely ignore it.
This tests with attributes correct (LIKELY), with attributes wrong (UNLIKELY) and without attributes (DEFAULT Behavior).

```
---Summary statistics for Branch Prediction with attribute LIKELY---
Sample mean cycles per test: 208202
Confidence interval: 207068-209336
Sample standard deviation: 40907.4
Tests used: 5000

---Summary statistics for Branch Prediction with attribute UNLIKELY---
Sample mean cycles per test: 436717
Confidence interval: 434746-438688
Sample standard deviation: 71103.2
Tests used: 5000

---Summary statistics for Branch Prediction with attribute DEFAULT BEHAVIOR---
Sample mean cycles per test: 210914
Confidence interval: 209475-212352
Sample standard deviation: 51878.2
Tests used: 5000
```

There is a statistically significant result that default behavior performs slightly worse surprisingly, though difference is very mild.
Using attributes incorrectly though tanks performance, more than 2x slower with the reversed attributes.
In a handwavy sense Makes sense its ~2x slower seeing its doubled the number of evaluations needed.

# plans

- Benchmarks for different types of containers, eg vector vs sparse set vs linked list vs colony/hive
- proper statistics with anova, levene test, residual normality checking and other stuff
- prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
- simd/vectorisation falls into SOA kinda
- false sharing messing with concurrency
- maybe LTO
