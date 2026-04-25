# benchmarks

some random benchmarks of questionable accuracy but is hopefully interesting
probably doesn't work on 32 bit systems due to hacky stuff!

# Benches!

Seems like standard deviation can be out of wack occasionally.
Not sure why but probably due to rare events like os jitter that can skew one sample to take far longer.

# Reserving a vector 15 gigabytes of capacity

For machines with an OS, a process runs inside a virtual address space which is abstracted away by the OS from physical memory addresses.
This is a necessity to have multiple processes running simultaneously, as each can reside inside their own virtual address space which the OS maps to physical memory addresses.
On a 64 bit linux system a process is typically allocated a 128 TiB large virtual address space, which mildly eclipses my laptops 16gb of RAM.
The key to why this works is the OS lazily maps virtual memory addresses to physical addresses, so it doesn't bother mapping it until the program actually accesses that memory.
<br>
Moving back to the vectors, when adding enough elements the vector runs out of capacity so it needs to trigger a resizing operation.
This means allocating more memory, then moving all its elements into its new larger space on the heap which is a costly operation.
This also breaks reference stability and iterator stability when adding elements, as pushing back could need a resizing operation.
A common performance trick to store n objects in say an empty vector,
is to reserve n capacity ahead of time so it resizes once at the start rather than multiple times as elements get pushed to the vectors back.
This is good for performance, but in cases where capacity is not perfectly known ahead of time, pushing one more object onto it will cause a nasty resize.
So how can wew avoid resizes without knowing the amount of storage needed ahead of time and without wasting lots of memory?
<br>
The answer is simply allocating 15 gigabytes of memory or a similarly excessive amount of memory to the vector.
Pushing back would be truly constant O(1), additionally references and iterators would stay stable when pushing to the back.
While there is a pointer to the memory, it hasn't been accessed yet, so while this uses 15 gigabytes of virtual memory, it uses no physical memory!
<br>
Below is a test between two vectors, one with a 15 GB reservation and one with no capacity reserved.
Both vectors get 5 million 8 byte objects (40MB total) pushed onto them.

```
---Summary statistics for No Reservation Vector---
Sample mean cycles per test: 8.34598e+07
Confidence interval: 8.11013e+07-8.58182e+07
Sample standard deviation: 8.29868e+06
Tests used: 50

---Summary statistics for 15 GB reservation vector---
Sample mean cycles per test: 3.37579e+07
Confidence interval: 3.29776e+07-3.45383e+07
Sample standard deviation: 2.74565e+06
Tests used: 50
```

This is ~2.5x faster which is an incredible speedup from avoiding resizing. While in this case the size was known ahead of time as its a test,
the key point is pushing any (realistic) number of elements doesn't triggers a resize and this hasn't wasted any more memory than required.
Proof of this is checking top (shows %cpu and memory usage) for either reservation type has both maxing at the same 0.3% memory usage respectively.
40 MB / 16000 MB (16GB) = 0.25% expected usage for just the push back loop, considering other memory used by the application, and 1 d.p. accuracy top rounding to 0.3% makes sense.
An important note is for both loops the vectors lifetime ends so the OS is free (depending on the memory controller) to,
but doesn't necessarily need to and in some cases can't unmap the memory.
In this case memory usage was visibly oscillating between 0.0 to 0.3 so in both cases it wasn't like the program was reusing the same physical memory mappings across loops.

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
Sample mean cycles per test: 95931.1
Confidence interval: 94846.3-97015.8
Sample standard deviation: 17480.4
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over uint8_t---
Sample mean cycles per test: 11486.2
Confidence interval: 11274.7-11697.7
Sample standard deviation: 3408.38
Tests used: 1000
```

Unbelievable ~9x speed increases due to SIMD, SOA is way faster here when operating on the 1 byte unsigned integers.
A more realistic example using floats seeing as these are coordinates.

```
---Summary statistics for Array of Structures Iteration over float---
Sample mean cycles per test: 171870
Confidence interval: 170979-172762
Sample standard deviation: 14368.1
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over float---
Sample mean cycles per test: 55092.7
Confidence interval: 54501.8-55683.7
Sample standard deviation: 9523.02
Tests used: 1000
```

Still ~3x speed increase which is incredible on the 4 byte floats.

```
---Summary statistics for Array of Structures Iteration over double---
Sample mean cycles per test: 197846
Confidence interval: 195237-200456
Sample standard deviation: 42051.9
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over double---
Sample mean cycles per test: 114971
Confidence interval: 113704-116238
Sample standard deviation: 20420.2
Tests used: 1000
```

Bit slightly under 2x speed increases which is still great for the 8 byte doubles.
Pattern here is fairly obvious, smaller data types take greater advantage of SIMD.
SIMD performs operations on multiple pieces of data in the CPU's register at once,
smaller data types lets the cpu pack more elements in the same register and operate on more data at the same time.

```
---Summary statistics for Array of Structures Iteration over long double---
Sample mean cycles per test: 2.34066e+06
Confidence interval: 2.29059e+06-2.39072e+06
Sample standard deviation: 806779
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over long double---
Sample mean cycles per test: 2.19956e+06
Confidence interval: 2.17646e+06-2.22267e+06
Sample standard deviation: 372367
Tests used: 1000
```

Performance gains almost completely disappear with the 16 byte long double.
SOA is still statistically significantly faster (p < 0.05) but its a very minor speed difference.
The difference could honestly just be due to better cache locality.
The cpu this was run on is based on the zen 4 architecture which has support for AVX-512 instruction set (512 bit aka 64 byte simd instructions),
though through a trick double pumping them through 256 bit registers.
Regardless even using AVX2 instructions for 256 bit registers you would expect to fit 2 long doubles (128 each),
but there's clearly not a sizeable enough performance increase.
On x86-64 linux, long double is 80 bit extended precision stored in 16 bytes, 10 data and 6 padding
(a lot of wasted space but it needs to be 16 byte aligned).
AVX2 and AVX512 have no support for 80 bit floats, just vectorizing 32 bit and 64 bit floats.
So the compiler uses scalar instructions and the very minimal performance increases are just due to the better cache locality which is interesting.

```
---Summary statistics for Array of Structures Iteration over 16 bit float---
Sample mean cycles per test: 499966
Confidence interval: 498174-501758
Sample standard deviation: 28875.4
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over 16 bit float---
Sample mean cycles per test: 541937
Confidence interval: 539957-543917
Sample standard deviation: 31900.5
Tests used: 1000
```

Then testing with

<br>

Obviously trying to vectorize code makes it far less readable and hard to maintain + understand,
but up to a 10x performance increase is a must have for some performance critical systems like computer graphics.

# Execution Policies

C++ 17 introduces some execution policies which can be used on some range based algorithms in the standard library.
These execution policies are:

- std::execution::seq (default sequential iteration)
- std::execution::par (allows parallel iteration)
- std::execution::par_unseq (allows parallel and non-sequential iteration)
- std::execution::unseq (C++20 allows non-sequential but not parallel iteration)

To use std::execution::par is to promise the function can be safely executed in parallel.
So while the implementation is free to use a parallel implementation it is also free not to.
par_unseq has a stronger guarantee that allows interweaving the execution of multiple function calls in the same thread.
This can let the code be vectorized and make use of SIMD if it is vectorization safe.

This benchmarked a few different scenarios using iteration over coordinates represented as SOA.
The first version was just a translation on the position by adding a vector.
All execution policies had the exact same performance, likely because the compiler was already vectorizing it for seq/unseq.
While parallel iteration was allowed operations were memory bandwidth bound, so the OS didn't parallelize operations.
This was modified then to do some more heavy (though unrealistic) operations on the data, needing to calc many sine functions.
A default generic indexed loop was also included which doesn't use a for_each function for the loop.
This is a zero cost abstraction so should be the same which is the case as seen below.

```
---Summary statistics for Generic Indexed Loop Execution Policy Benchmark---
Sample mean cycles per test: 1.51148e+08
Confidence interval: 1.48761e+08-1.53535e+08
Sample standard deviation: 8.39785e+06
Tests used: 50

---Summary statistics for Sequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.49426e+08
Confidence interval: 1.47238e+08-1.51615e+08
Sample standard deviation: 7.70051e+06
Tests used: 50

---Summary statistics for Unsequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.47965e+08
Confidence interval: 1.45733e+08-1.50197e+08
Sample standard deviation: 7.85318e+06
Tests used: 50

---Summary statistics for Parallel Execution Policy Benchmark---
Sample mean cycles per test: 1.98117e+07
Confidence interval: 1.91472e+07-2.04762e+07
Sample standard deviation: 2.33812e+06
Tests used: 50

---Summary statistics for Parallel and Unsequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.91419e+07
Confidence interval: 1.86417e+07-1.96421e+07
Sample standard deviation: 1.75997e+06
Tests used: 50
```

While explicitly allowing vectorization led to no improvement,
the parallel execution policies were ~7.5x faster which is quite nice.

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
