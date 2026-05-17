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

Moving back to the vectors, adding enough elements to a vector can have its size exceed its capacity, this requires the vector to resize.
This means allocating more memory, then moving all its elements into its new larger space on the heap which is a costly O(n) operation.
This also breaks reference stability and iterator stability when adding elements, as pushing back could trigger a resize, which moves the address of elements.
To get around this partially there is a common performance trick to add n objects into say an empty vector.
This is done by reserving n capacity ahead of time so it resizes once at the start rather than potentially multiple times as elements get pushed to the back of the vector.
While nice, in many cases the number of elements isn't known ahead of time so guessing k elements may help but if size reaches k+1 a resize will take k costly copies of the object.
So how can we avoid resizes, without knowing the amount of storage needed ahead of time and without wasting lots of memory?
<br>

The answer is a <s>linked list</s> allocating 15 gigabytes of memory or a similarly excessive amount of memory to the vector.
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

```
// compiled using just 15gb reserve vector
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
  39455 tristan   20   0   15.0g  16884   4144 R 100.0   0.1   0:04.21 benchmarks

// compiled using just no reserve vector
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
  42979 tristan   20   0   18468  13404   4036 R  94.9   0.1   1:09.86 benchmarks
```

This is an interesting output from top. VIRT is the amount of virtual memory the process is using, while RES (resident set size) is the amount of physical RAM the process is actually using at that point in time. As expected the no reserve vector is using up 15gb of virtual memory but as seen the resident set size is small. Increasing memory usage to a few gigabytes so the process is slow, with the no reserve vector you can see that virtual memory usage is no greater than 2x larger than the current RES. This makes perfect sense since compiling it with gcc, vectors have a 2x resize ratio.

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

<br>

Obviously trying to organize data in a way operations can be vectorized can be very counterintuitive,
but up to a 10x performance increase is a must have for some performance critical systems.
Game engines make heavy use of this through using an ECS (entity component system).

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
Sample mean cycles per test: 1.49309e+08
Confidence interval: 1.47017e+08-1.51601e+08
Sample standard deviation: 8.06464e+06
Tests used: 50

---Summary statistics for Sequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.45872e+08
Confidence interval: 1.43686e+08-1.48058e+08
Sample standard deviation: 7.69117e+06
Tests used: 50

---Summary statistics for Unsequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.49719e+08
Confidence interval: 1.47966e+08-1.51473e+08
Sample standard deviation: 6.17008e+06
Tests used: 50

---Summary statistics for Parallel Execution Policy Benchmark---
Sample mean cycles per test: 1.85782e+07
Confidence interval: 1.81993e+07-1.89572e+07
Sample standard deviation: 1.33327e+06
Tests used: 50

---Summary statistics for Parallel and Unsequenced Execution Policy Benchmark---
Sample mean cycles per test: 1.58457e+07
Confidence interval: 1.52099e+07-1.64815e+07
Sample standard deviation: 2.23711e+06
Tests used: 50
```

Explicitly allowing vectorization through the unsequenced execution policy had ~ no impact on speed.
This makes sense since the compiler should really be able to figure out it can get vectorized in this case.
A parallel execution policy though has ~7.85x speedup which is quite nice.
A parallel unsequenced execution policy though lead to further improvements, with it being ~9.2x faster.
This is ~1.17x faster than the parallel execution policy which is great.

# SSO

One of the key differences between std::string and a c style string made through a char array is a std::string is resizable.
It does this through managing a heap allocated resource, working similar to a vector with it resizing to fit the characters that need storage.
std::string though is often considered badly performant due to this though as it can lead to a lot of costly heap allocations, which is the case but there is a trick for small strings.
A naively implemented string implementation would look something like a vector, with a pointer to its heap allocated resource and two size_t's, capacity of the resource and current size.
This is logical but something like std::string = ""; would result in an expensive heap allocation to store just a null terminator.
The trick involves type punning, what if rather than heap allocating memory, you rather use the 16 bytes from the pointer + capacity to store it.
Before an explanation, this is an example of how it looks blind when benchmarking.

```
---Summary statistics for String of size 15---
Sample mean cycles per test: 113942
Confidence interval: 113099-114785
Sample standard deviation: 7419.43
Tests used: 300

---Summary statistics for String of size 16---
Sample mean cycles per test: 114166
Confidence interval: 113365-114967
Sample standard deviation: 7048.83
Tests used: 300

---Summary statistics for String of size 17---
Sample mean cycles per test: 322075
Confidence interval: 320840-323310
Sample standard deviation: 10867.1
Tests used: 300

---Summary statistics for String of size 18---
Sample mean cycles per test: 317864
Confidence interval: 316757-318970
Sample standard deviation: 9740.29
Tests used: 300
```

This can be seen incredibly cleanly, an increase of size held to 17 bytes from 16 bytes results in a ~2.8x drop in performance!
This is a clear drop off between stack and heap allocation "modes", this is called SSO (small string optimization) as you avoid needing a heap allocation for small strings.
Note that by a string of size 16, I mean it holds 16 bytes total which includes the null terminator, not 16 characters + "hidden" 17th character null terminator.
So the dropoff starting at 16 shows the String uses at max a 16 byte char stack buffer inside it.
This used the libstdc++ standard library implementation where a string is 32 bytes.
A simplified version of how its implemented is along these lines.

```
class String {

    class Union {
        size_t capacity;
        char stackbuf[16];
    };

    char* data;
    size_t size;
};
```

The union member only takes up the space of its largest member which is the 16 byte stackbuf.
Here this reuses the 8byte capacity field with the 16 byte charbuf.
This doesn't typepun both data + capacity, just capacity so this "wastes" 8 bytes when its using a heap allocated buffer which only needs 2 size_t's and the pointer.
A benefit is checking mode is simple, if data points to the stackbuf you are using it, if the data is pointing to something else you have a heap allocated buffer.
<br>

There are alternative implementations also like the libc++ one type punning both size_t's and the data pointer for 23 characters of storage and a 24 byte total size.
A probably inaccurate simplification is below:

```
class String {
    class Union {
        struct {
            char* data;
            size_t size;
            size_t capacity;
        } heapMode;

        struct {
            std::array<char, sizeof(heapMode) - 1> stackbuf;
            uint8_t size;
        } stackMode;
    };
};
```

The issue is you need a way to check which mode the string is in, should data be interpreted in stack or heap mode?
Note even if both size's were 8 bytes so they could get interpreted normally, the string can reserve memory or it could've downsized while having memory allocated prior.
This can be done through the most insignificant bit of a set address which would overlap with size in stack mode.
In stack mode this would reduce the state's size can represent down to 127 from 255.
This is fine though since in stack mode it can only really hold 22 character by the user (23 char's total due to null terminator).
In heap mode the capacity field would drop to only being able to hold 2^63 states which is still an excessive amount.
This is a very interesting optimization making use of a lot of tricks.
A more accurate explanation is [here](https://joellaity.com/2020/01/31/string.html).

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
Sample mean cycles per test: 1.12059e+06
Confidence interval: 1.11124e+06-1.12994e+06
Sample standard deviation: 82293.3
Tests used: 300

---Summary statistics for Branch Prediction Unsorted Version---
Sample mean cycles per test: 2.13579e+06
Confidence interval: 2.13264e+06-2.13894e+06
Sample standard deviation: 27723.8
Tests used: 300
```

As seen decent ~90% performance increase by a seemingly strange optimization for the hardware.

# Random access of char vs uint8_t vs bool vectors

C++ uses a template specialization for bool vectors where it does bit packing of 8 bools in a byte.
This is good for memory but as memory is addressed by the byte this leads to some issues:

- Runtime overhead when reading / writing as bit manipulation is needed to access the specific bit
- Can't take a reference to a bool in a vector
- Accessing a bool in a vector returns a proxy object which is used to access it

Would expect random access to be the same for char and uint8_t vectors, longer for bool vectors.
Benchmarks show this is usually the case, this is a bench of accessing 1000 random indices from a 10000 element array.
~ equal for char and uint_8, ~38% longer for bool vectors.

```
---Summary statistics for bool vector random access---
Sample mean cycles per test: 1441.61
Confidence interval: 1417.43-1465.78
Sample standard deviation: 389.581
Tests used: 1000

---Summary statistics for char vector random access---
Sample mean cycles per test: 1045.53
Confidence interval: 1026.33-1064.73
Sample standard deviation: 309.429
Tests used: 1000

---Summary statistics for uint8_t vector random access---
Sample mean cycles per test: 1034.74
Confidence interval: 1025.11-1044.37
Sample standard deviation: 155.214
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
Was expecting xoroshiro128+ to be substantially faster and it is, ~3.17x faster number generation is very welcome.
Xoroshiro also has better statistical properties.

```
---Summary statistics for Xoroshiro128+ RNG---
Sample mean cycles per test: 2.52777e+06
Confidence interval: 2.5216e+06-2.53394e+06
Sample standard deviation: 54302.4
Tests used: 300

---Summary statistics for Mersenne twister RNG---
Sample mean cycles per test: 7.98123e+06
Confidence interval: 7.92463e+06-8.03783e+06
Sample standard deviation: 498167
Tests used: 300
```

# LIKELY / UNLIKELY Attributes

C++ 20 introduces the [[likely]] and [[unlikely]] attributes which are "hints" to inform the compiler if a path of execution is more or less likely than another.

```

if constexpr (A == Attribute::UNLIKELY) {
    if (number > SIZE_NEEDED_FOR_SUCCESS) [[unlikely]] { // lie 95% is unlikely
        successes += 1;
    } else if (number == SIZE_NEEDED_FOR_SUCCESS) [[likely]] { // lie ~0% is likely
        equalities += 1;
    }
}

```

This benchmarks 3 scenarios like above, the randomly generated number has a ~95% chance to be > size_needed_for_success.
For the number to be equal this was exceedingly rare (1/100000) so it realistically will never happen.
Evaluating the > condition first would allow short circuiting 95% of the time which skips needing a second evaluation,
while evaluating the == condition first would basically never short circuit so it would ~always need 2 evaluations.
<br>

This uses the [[unlikely]] attribute and [[likely]] attribute to lie to the compiler.
If it trusts me this would theoretically tank performance as it would evaluate the unlikely condition first thinking that path of execution was "more likely".
This is obviously just a hint to the compiler, definitely compiler specific and the compiler could just completely ignore it (this was done using GCC).
This tests with attributes correct (LIKELY), with attributes wrong (UNLIKELY) and without attributes (DEFAULT Behavior).

```
---Summary statistics for Compiler Branch Prediction with attribute LIKELY---
Sample mean cycles per test: 248460
Confidence interval: 247094-249827
Sample standard deviation: 49292.7
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute UNLIKELY---
Sample mean cycles per test: 551751
Confidence interval: 548819-554683
Sample standard deviation: 105757
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute DEFAULT BEHAVIOR---
Sample mean cycles per test: 294556
Confidence interval: 293124-295988
Sample standard deviation: 51658.3
Tests used: 5000
```

There is a statistically significant result that default behavior is ~18.5% slower than when properly using likely.
Using attributes incorrectly though tanks performance, 2.22x slower with the reversed attributes.
In a handwavy sense Makes sense its ~2x slower seeing its doubled the number of evaluations needed.
For fun I added a simple branchless version like this expecting it to improve performance by eliminating branch mispredictions.

```
// A manual branchless implementation
if constexpr (A == Attribute::BRANCHLESS) {
    successes += (number > SIZE_NEEDED_FOR_SUCCESS);
    equalities += (number == SIZE_NEEDED_FOR_SUCCESS);
}
```

The results were initially surprising but make a lot of sense.

```
---Summary statistics for Compiler Branch Prediction with attribute LIKELY---
Sample mean cycles per test: 248460
Confidence interval: 247094-249827
Sample standard deviation: 49292.7
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute UNLIKELY---
Sample mean cycles per test: 551751
Confidence interval: 548819-554683
Sample standard deviation: 105757
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute DEFAULT BEHAVIOR---
Sample mean cycles per test: 294556
Confidence interval: 293124-295988
Sample standard deviation: 51658.3
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute BRANCHLESS VERSION---
Sample mean cycles per test: 398114
Confidence interval: 396564-399664
Sample standard deviation: 55893.9
Tests used: 5000
```

The branchless version is actually significantly slower than the default version that had direct conditional branches,
However its still a bit faster than the branch prediction with the incorrect [[unlikely]].
In retrospect this should've been completely expected though.

```
if (number > SIZE_NEEDED_FOR_SUCCESS) [[likely]] {
    successes += 1;
} else if (number == SIZE_NEEDED_FOR_SUCCESS) [[unlikely]] {
    equalities += 1;
}
```

The branch predictor will likely assume number > SIZE_NEEDED_FOR_SUCCESS is likely,
unless it mispredicts and needs to clear the pipeline the else condition check and write isn't executed.
In the likely version I will have 1 write and 1 boolean logic operation most of the time with the occasional branch misprediction.
In the unlikely version I am practically guaranteed 2x writes and 2x boolean logic operations with the occasional branch misprediction.
In the branchless version while there is no cost of misprediction, I will have 2 writes and 2 boolean logic ops always time.
Because the branch prediction is right ~95% of the time the branch misprediction isn't that costly while the effective short circuiting is big.
So theoretical decreasing the chance of if (number > SIZE_NEEDED_FOR_SUCCESS) [[likely]] to something like 70% from 95% could slow it down enough for branchless to come ahead.

```
---Summary statistics for Compiler Branch Prediction with attribute LIKELY---
Sample mean cycles per test: 951195
Confidence interval: 946995-955395
Sample standard deviation: 151482
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute UNLIKELY---
Sample mean cycles per test: 1.19977e+06
Confidence interval: 1.19582e+06-1.20371e+06
Sample standard deviation: 142179
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute DEFAULT BEHAVIOR---
Sample mean cycles per test: 1.10807e+06
Confidence interval: 1.10447e+06-1.11168e+06
Sample standard deviation: 129937
Tests used: 5000

---Summary statistics for Compiler Branch Prediction with attribute BRANCHLESS VERSION---
Sample mean cycles per test: 402328
Confidence interval: 400537-404118
Sample standard deviation: 64589.1
Tests used: 5000
```

Doing this with a 70% chance for the first condition, short circuiting happens less and costs from failed branch predictions wil increase.
We see branch prediction costs have dominated the short circuiting benefit with the branchless version being fastest by a ~2.36x factor compared to 2nd place (likely).
Furthermore the unlikely version is only ~1.26x slower rather than 2.22x slower as the short circuiting benefits have dropped off and branch misprediction costs start to dominate in time used.

# Reorganizing a struct for less memory usage

On a hardware level, data is physically divided into cache lines which are typically 64 byes long. When the CPU fetches memory from RAM it fetches that memory by the whole cache line, the cache line is the smallest unit of data transferrable between RAM and CPU cache. This is good because there is a spacial locality to data access, meaning getting some nearby data with what you want can help minimize future cache misses. Too large of a cache line though and you pick up more irrelevant data which could displace actually useful stuff from the cpu's cache so there's a balance. If you had an 8 byte piece of data, but it started at the end of one cache line and ended in the other when accessing it you would need both cache lines. This is called an unaligned access, in the worst case old CPU's don't support unaligned memory access and can crash, modernly this just has a speed penalty as the processor needs to read/write from two different cache lines and increases memory access latency. An additional thing is the whole cache line is mapped to a single page, not spanning across two. There is no guarantee that data will exist on one page if this piece of data is split across cache line boundaries though this would be fairly unlucky it could be pretty costly. On modern CPU's the effect of unaligned access is significantly less impactful.
<br>

A way to get around this is through aligning data. For example a cache line is typically 64 bytes, though some chips implement different size cache lines but they are all of 2^n in size. If you wanted to find if a memory address was "safe" to place 4 bytes of data without it crossing cache lines, you could check if that address was divisible by 4. [0:63] wouldn't allow data to start at 61-63 as some bytes would cross a cache line. 60 which is divisible by 4 would be safe as it would take up [60, 61, 62, 63]. On 64 bit linux systems a long double is typically on 10 bytes of size. A 4 byte alignment obviously wouldn't work since starting at address 60 would lead to it overflowing, and a 10 byte alignment would also mark 60 as fine for it to start which it isn't. Rather you need the smallest factor of 64 which is as large as your piece of data for proper alignment. A good property of c = 2^n, is its factors are 2^x for 0 <= int x <= n. So a 10 byte long double would be wasting 6 bytes of space as it needs to be properly 16 byte aligned.

```

struct IntDoubleInt {
    int int_val_1; // needs to be 4 byte aligned [0:3]
    double double_val; // needs to be 8 byte aligned [8:15]
    int int_val_2; // needs to be 4 byte aligned [16:19]
}; // struct has 8 byte alignment, but it ends at 19 (20 bytes used), this isn't a multiple of 20
// so we need to add some padding at the end. So it would take [0:23] or 24 bytes total

struct IntIntDouble {
    int int_val_1; // needs to be 4 byte aligned [0:3]
    int int_val_2; // needs to be 4 byte aligned [4:7]
    double double_val; // needs to be 8 byte aligned [8:15]
}; // struct has the alignment of its largest elements alignment which is 8 bytes here. So [0:15]
// works out with no end padding as thats a multiple of 8.

```

This is an example in practice, when creating a struct / class the standard guarantees that the order you define members in is the same order they are laid out in memory. For the IntDoubleInt the struct itself is 8 byte aligned because the struct has the alignment of the largest alignment of its members. This means the struct is starting at an address which is a multiple of 8. We then add an int, a multiple of 8 is a multiple of 4 so no padding needs to be considered here. But now the next address is a 4 offset from a multiple of 8, so to add a double we insert padding which is just empty space. We can then insert the 4 byte int. This has used up 4 bytes from the first int, 4 from padding, then another 8 from the float, 4 from the int which adds up to 20 bytes. But the data needs to take up the whole alignments worth of space so it adds 4 bytes of padding at the end to keep the struct aligned. This results in it taking 24 bytes total. In the IntIntDouble case there is no alignment issues though so its only taking up 16 bytes, this is 1/3 less space used from good ordering of members. Because the standard guarantees order you define members in is the same order they are laid out in memory the compiler is not free to reorder them, its completely up to the programmer.

```

static_assert(sizeof(IntIntDouble) == 16);
static_assert(sizeof(IntIntDouble) == (sizeof(int) + sizeof(int) + sizeof(double)));

static_assert(sizeof(IntDoubleInt) != 16);
static_assert(sizeof(IntDoubleInt) != (sizeof(int) + sizeof(int) + sizeof(double)));
static_assert(sizeof(IntDoubleInt) == 24);

```

Because of that these static asserts pass.

# Empty Base Optimization, limitations and [[no_unique_address]]

In C++ every object is guaranteed a unique memory address during its lifetime.
Because of this the size of an empty struct/class or a functor (lambda) with no capture clause is 1.
This could lead to issues though, think about a vector which needs an instance of an allocator.
std::allocator is completely stateless, so having it as a member would result in it taking up a byte of space.
This wouldn't be ideal obviously as it would bloat the sizes of objects for no real reason.
This is solved through empty base optimization.
As a base of a class does not require a unique address, it shares address with the first non static member.

```

struct Empty {};
struct BaseOptimization : public Empty {
    std::byte b;
};
static_assert(sizeof(BaseOptimization) == 1);

struct NoBaseOptimization {
    std::byte b;
    Empty e;
};
static_assert(sizeof(NoBaseOptimization) == 2);

struct FailedBaseOptimization : Empty {
    BaseOptimization b;
};
static_assert(sizeof(FailedBaseOptimization) == 2);

```

Another interesting thing is empty base optimization fails if the first non static member (whom address it overlaps) is the same type, or has a base which is the same type as that member.

```

struct CorrectBaseOptimization : Empty {
    std::byte b;
    Empty e;
};
static_assert(sizeof(CorrectBaseOptimization) == 2);

struct IncorrectBaseOptimization : Empty {
    Empty e; // first non static member is of same type, so can't overlap the address
    std::byte b;
};
static_assert(sizeof(IncorrectBaseOptimization) == 3);

```

A C++ 20 addition is the attribute [[no_unique_address]] which loosens the guarantee for a classes member to have a unique memory address.
This would allow something similar to empty base optimization, but for empty members.

```

struct MemberOptimization {
    std::byte c;
    // no unique address allows the compiler to not give e an unique memory address since it has no members
    [[no_unique_address]] Empty e;
};
static_assert(sizeof(MemberOptimization) == 1);
// offsetof gets the offset of a member from the start of the object.
static_assert(offsetof(MemberOptimization, e) == offsetof(MemberOptimization, e));

```

This allows the cutting of one byte from this struct which is good since it doesn't really need a unique address.
This is interesting though that its been added as an attribute, as compilers are allowed to ignore attributes.
This means the memory layout of objects could be dependent on compiler which is somewhat messy.
GCC and CLANG will generally follow it, but MSVC always ignores it due to it breaking ABI as it changes object layouts.

# plans

- Benchmarks for different types of containers, eg vector vs sparse set vs linked list vs colony/hive
- proper statistics with anova, levene test, residual normality checking and other stuff
- prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
- simd/vectorisation falls into SOA kinda
- false sharing messing with concurrency
- maybe LTO

```

```
