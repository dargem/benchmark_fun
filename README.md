# Benchmarks

Some random benchmarks of questionable accuracy, but I hope they're interesting.
This probably doesn't work on 32-bit systems due to some hacky code.
It also contains information on various performance-related C++ language features.

# Table of Contents

- [Ring Buffer Optimizations Through Minimizing Coherence Traffic](#ring-buffer-optimizations-and-minimizing-coherence-traffic)

- [Curiously Recurring Template Pattern (CRTP)](#crtp)

- [Structure of Arrays vs Array of Structures (with SIMD)](#structure-of-arrays-soa-vs-array-of-structures-aos-simd-test)

- [Zero Cost Compile Time Storage Specialization](#zero-cost-compile-time-storage-specialization)

- [Sorting To Help With Branch Prediction](#sorting-to-help-with-branch-prediction)

- [LIKELY / UNLIKELY Attributes & A Branch Prediction Tangent](#likely--unlikely-attributes-and-a-branch-prediction-tangent)

- [Heap Allocation Costs & Alternatives](#heap-allocation-costs--alternatives)

- [Small String Optimization (SSO)](#sso)

- [Reserving 15 GB Of Capacity For A Vector](#reserving-15-gb-of-capacity-for-a-vector)

- [Costs & Dangers Of Unaligned Access](#costs-and-dangers-of-unaligned-access)

- [Mersenne Twister (MT19937) vs Xoroshiro Family RNGs (and a SIMD Accelerated One)](#mersenne-twister-mt19937-vs-xoroshiro-family-rngs-and-a-simd-accelerated-one)

- [Execution Policies](#execution-policies)

- [Random Access Of char vs uint8_t vs bool Vectors](#random-access-of-char-vs-uint8_t-vs-bool-vectors)

- [Padding To Avoid Unaligned Access & Reorganizing Structs For Less Memory Usage](#padding-to-avoid-unaligned-access-and-reorganizing-a-struct-for-less-memory-usage)

- [Empty Base Optimization, Limitations & [[no_unique_address]]](#empty-base-optimization-limitations-and-no_unique_address)

- [Compile Time String Hashing](#compile-time-string-hashing)

- [Exceptions To The As-If Rule](#exceptions-to-the-as-if-rule)

# Benches

The standard deviation can be out of whack occasionally. This is likely due to rare events such as OS jitter that can skew individual samples.

# Reserving 15 GB of capacity for a vector

Processes run in a virtual address space that the OS maps to physical memory. On a 64‑bit Linux system, a process is typically allocated a 128 TiB virtual address space, which if somewhat larger than my laptop's 16 GB of RAM. The key to how this doesn't lead to excessive memory usage is through how the OS lazily maps virtual addresses to physical pages. Essentially memory isn't backed physically by RAM (or disk) until it's actually accessed.

When a vector grows past its capacity it must resize, allocate more memory, and move all elements into the new space, which is an expensive O(n) operation. Resizing also breaks reference and iterator stability. A common performance trick is to call `reserve(n)` so the vector grows once instead of many times if you know the number of elements to add ahead of time.

If you don't know the final size, another trick is to reserve an excessive amount of virtual memory up front (e.g., 15 GB). With such a large amount of memory allocated the vector will never need to resize, so push_back operations become truly O(1) not just amortized O(1). References and iterators also remain stable (for push_back operations at least) and excessive RAM isn't used as the reserved virtual memory isn't backed by RAM until its actually touched.

Below is a test between two vectors: one with a 15 GB reservation and one with no capacity reserved. Both vectors receive 5 million 8‑byte objects (40 MB) pushed onto them.

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

This is ~2.5x faster, which is a large speedup from avoiding resizing. While in this case the size was known ahead of time since it's a test, the key point is that pushing any realistic number of elements won't trigger a resize and this hasn't wasted additional memory. This means if you have a vector and you don't know how many objects you need to push to it ahead of time, you could reserve an excessive amount of space without worries and never have to worry about resizes.

Some proof is `top` (shows %CPU and memory usage) reports both reservation types maxing at about 0.3% memory usage. 40 MB / 16,000 MB (16 GB) ≈ 0.25% expected usage for the push-back loop; accounting for other memory used by the application and `top`'s rounding to 1dp, 0.3% is reasonable.

```
// compiled using 15 GB reserved vector
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
  39455 tristan   20   0   15.0g  16884   4144 R 100.0   0.1   0:04.21 benchmarks

// compiled using no-reserve vector
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
  42979 tristan   20   0   18468  13404   4036 R  94.9   0.1   1:09.86 benchmarks
```

This output from `top` is informative. `VIRT` shows the virtual memory size and `RES` shows the resident set size (physical RAM). The reserved vector shows a large virtual size but a small `RES`. An interesting but expected thing is the no-reserve vector's virtual memory usage is no more than roughly 2× the current `RES`, which makes sense knowing libstdc++ uses a 2x growth strategy.

# Structure of Arrays (SoA) vs Array of Structures (AoS) (SIMD test)

This benchmark iterates over a list of entities. Each entity contains: `attack`, `defense`, `health`, `x`, `y`, and `z`. The benchmark has each iteration simply increment `x`, `y`, and `z` by 1.

An Array of Structures (AoS) approach is the standard way to think about laying out data. Here it would be typically a `std::vector<Entity>` where the Entity class is say a struct with each of those members. A Structure of Arrays (SoA) layouot stores each field in its own vector (e.g., vectors for `attack`, `defense`, `health` and etc) where index N corresponds to an entity. If when iterating over entities you just want to say check every entities health is > 0, the cache benefits are fairly obvious as you no longer need to load in the entities currently unused data into the cache which would cause thrashing. The second main benefit however is that this enables vectorization, where the CPU is able to perform one operation on multiple pieces of data in parallel. This is called SIMD (single instruction, multiple data) and it leads to massive performance increases.

An example is you have entities which have a velocity and an acceleration field laid out in SoA style. And say 1 second has passed so now we want to have each velocity += acceleration. In a SoA layout this means we have a velocity array and an acceleration array where index N corresponds to an entities velocity and acceleration. We want to use SIMD to accelerate this addition by doing that one addition instruction on multiple entities at once. The compiler can do this but it can also be a bit iffy and not, so we can use SIMD intrinsics which let you use specific CPU instructions without having to actually write assembly. They are surprisingly simple a good guide is [here] (https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#ig_expand=7113,140,119&techs=AVX_ALL).

Data must first be loaded into special vector registers which are far larger than standard ones, for this example we will use 32 byte (256 bit) vector registers. And say the cpu supports AVX2 which most modern cpu's with x86 architecture likely will. Say velocity and acceleration are both doubles (8 bytes), this means we can have one vector register filled with 4 velocities, and another vector register with 4 accelerations. Then we can use `__m256d output = _mm256_add_pd(a, b);` Which will return a vector register which has added each paired velocity and acceleration together. We can then overwrite the 4 contiguous doubles from our original velocity vector with this register and we have successfully used one instruction on 4 pieces of data simultaneously. We can push this further, using single precision floats would let us pack 8 elements rather than 4 per 256 bit register. If your CPU is modern it may have 256 bit vector registers and you could be doing one operation on 16 pieces of data. If your CPU supports vectorization of half precision floats you could be doing one operation on 32 pieces of data simultaneously. If your CPU supports AVX512-BW instruction set you could be using SIMD instructions on data the size of a byte, and doing what would be 64 operations at once with 512 bit vector registers. Note sizes are implementation-defined; on my machine floats, doubles and `long double` are 4, 8 and 16 bytes respectively. `long double` size varies by platform and ABI. My machine has 256 bit vector registers and doesn't support the AVX512-BW instruction set which is a pity.

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

About ~9x speedup due to SIMD: SoA is much faster here when operating on 1‑byte unsigned integers.
This makes sense since my laptop doesn't support the AVX512-BW instruction set so its probably using epi32 (4 byte integer) instructions.
With 256 bit vector registers we would be doing 8 operations simultaneously and gain some benefits from better cache locality.
So taking a gander this speedup makes sense.
Below is a more realistic example using floats (coordinates).

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

Still ~3x speed increase on 4‑byte floats.

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

Slightly under a ~2x speed increase for 8‑byte doubles, which is still a solid gain. The pattern is clear, smaller data types generally gain more from SIMD because the CPU can pack more elements into the same SIMD registers and operate on more data in parallel.

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

Performance gains largely disappear for 16‑byte `long double`. SoA is still statistically faster (p < 0.05), but the difference is small and probably due to better cache locality. This makes sense, AVX2/AVX-512 does not support vectorizing long doubles so the compiler would emit scalar instructions for `long double`. As such there would be no vectorization benefits.

```
---Summary statistics for Array of Structures Iteration over 16‑bit float (half precision)---
Sample mean cycles per test: 499966
Confidence interval: 498174-501758
Sample standard deviation: 28875.4
Tests used: 1000

---Summary statistics for Structure of Arrays iteration over 16-bit float---
Sample mean cycles per test: 541937
Confidence interval: 539957-543917
Sample standard deviation: 31900.5
Tests used: 1000
```

Here we see minimal performance gains from an array of half precision 16 bit floats, despite 32 bit floats getting a 3x speedup.
This is because my CPU does not support the instructions for vectorizing half precision floats so the compiler falls back to slow scalar instructions.
This is pretty awful since its a massive tank in performance from using half precision rather than single precision floats.
There is a statistically significant result SoA is faster but this is likely all due to cache locality.

<br>

Organizing data for vectorization can be counterintuitive, but up to a 10x performance increase makes it worthwhile for performance‑critical systems. Game engines commonly use ECS (Entity Component System) patterns to exploit these gains.

# Heap Allocation Costs & Alternatives

Dynamic memory allocation is commonly thought of as relatively expensive. Most system allocators (e.g., `new`, `malloc`) request large blocks from the OS (for example using `mmap` on Linux) and split them into smaller chunks. They often manage a free list (a linked list of free chunks). A caller requests N bytes, and the allocator traverses the free list to find a suitable sized block. If needed, it can split a larger block and allocate pieces of it, or request more memory from the OS. Over time, allocations can fragment memory into small non‑contiguous chunks, which makes finding a suitably large block slower. This leads to more time taken for allocations as it starts taking longer to find a suitably sized free chunk, especially if the allocation requires a large amount of data.

By contrast, allocating on the stack is just an increment of the stack pointer. A common solution for heap allocation cost is to use custom allocators such as arena allocators. An arena works like a stack: allocate a large block up front (similar to the reserve trick), keep a start pointer and a current pointer, then bump the current pointer by the allocation size.

```
template <typename T>
    requires std::same_as<std::remove_cvref_t<T>, T> && std::is_trivially_destructible_v<T>
T* allocate(size_t n) {
    // This allocates aligned memory so we make sure current ptr is aligned here
    current +=
        (alignof(T) - reinterpret_cast<uintptr_t>(current) % alignof(T)) % alignof(T);
    T* ptr = reinterpret_cast<T*>(current);
    current += sizeof(T) * n;
    return ptr;
}
```

This example is simple and fast but lacks flexibility in that you can't free individual allocations. You can only advance the pointer or reset the arena to free everything (LIFO semantics). This ties object lifetimes to the arena: calling `reset()` reclaims all memory in O(1) by resetting the current pointer back to the beginning. This pattern is useful for per‑frame allocations in a game engine for example as the lifetime of everything allocated in that frame lives and dies in it.

Note my `allocate` function requires `T` to be trivially destructible because destructors aren't called when resetting the arena. Allocating objects with nontrivial destructors that manage resources can produce incorrect behavior.

I ran a benchmark allocating 50,000 4‑byte objects using my arena implementation vs `new`. This benchmark measured allocation only (no deletion).

```
---Summary statistics for New Allocator---
Sample mean cycles per test: 2.46151e+06
Confidence interval: 2.45673e+06-2.46628e+06
Sample standard deviation: 76992.5
Tests used: 1000

---Summary statistics for Arena Allocator---
Sample mean cycles per test: 114167
Confidence interval: 113007-115328
Sample standard deviation: 18702.6
Tests used: 1000
```

We see a ~21.6x speedup, which is expected. But if you allocate on the heap you typically also free it eventually; this benchmark also considers freeing 50,000 integers.

```
---Summary statistics for New Allocator---
Sample mean cycles per test: 2.82874e+06
Confidence interval: 2.82396e+06-2.83351e+06
Sample standard deviation: 76985.6
Tests used: 1000

---Summary statistics for Arena Allocator---
Sample mean cycles per test: 116649
Confidence interval: 115762-117536
Sample standard deviation: 14298.2
Tests used: 1000
```

We see ~25.25x speedup in this test. The arena allocator is essentially the same speed as before since resetting the arena is O(1) just changing the address pointed to of the current pointer.
To model more realistic behavior, I ran another benchmark where allocation sizes were uniformly distributed in [1, 256] bytes. As the free list fragments, fulfilling larger allocations becomes harder, and `new` slows further. So I hypothesized the new allocator would get a lot slower.

```
---Summary statistics for New Allocator---
Sample mean cycles per test: 5.5548e+06
Confidence interval: 5.53673e+06-5.57288e+06
Sample standard deviation: 291335
Tests used: 1000

---Summary statistics for Arena Allocator---
Sample mean cycles per test: 116631
Confidence interval: 116099-117164
Sample standard deviation: 8581.14
Tests used: 1000
```

The arena allocator takes essentially the exact same speed since we're just changing how much the current pointer increments. This means larger objects don't take more time to allocate. `new` however is considerably slower, and we have now achieved a 47.6x speedup over `new`.

# Execution Policies

C++17 introduced execution policies for some algorithms in the standard library. The common policies are:

- `std::execution::seq` - sequenced (default)
- `std::execution::par` - may execute in parallel
- `std::execution::par_unseq` - may execute in parallel and unsequenced (vectorization allowed)
- `std::execution::unseq` - unsequenced (vectorization allowed, not parallel by itself)

Using `std::execution::par` indicates the function can safely be executed in parallel; the library may or may not use parallelism. `par_unseq` gives the implementation more freedom (parallel + unsequenced), which can enable both parallelization and vectorization when safe.

This benchmarks several scenarios iterating over coordinates represented as SoA. The first test simply translated positions (add a vector). All execution policies had similar performance, likely because the compiler already vectorized the loop and memory bandwidth limited parallelism. I then added a heavier workload (computing many `sin` calls) to exercise parallel execution. A generic indexed loop (no `for_each`) was also added as a sanity check. `for_each` is a zero cost abstraction so it should be the same speed.

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

Explicitly allowing unsequenced execution had little effect here, since the compiler is smart enough to already vectorize the loop. A parallel execution policy yielded ~7.85x speedup, while `par_unseq` improved further to ~9.2x (~1.17× faster than `par`) which was interesting.

# Ring buffer optimizations and minimizing coherence traffic

Inspired by https://rigtorp.se/ringbuffer/.
For a single‑producer, single‑consumer queue, a ring buffer is an efficient data structure. One thread produces (pushes) items into the FIFO and another consumes (pops) them. Being lock‑free, it avoids expensive concurrency primitives.
The basic idea in pseudocode:

```
std::vector<DataType> data;
alignas(64) std::atomic<size_t> read_idx{}; // Where the next read will go
alignas(64) std::atomic<size_t> write_idx{}; // Where the next write will go

when write_idx == read_idx the queue is empty
when ((write_idx + 1) % data.size() == read_idx) the queue is full

bool push(DataType d) {
    // check the queue is not full, if it isn't push it into data at write_idx and advance write_idx by 1
    // returns false if the queue is full as the push failed
}

bool pop(DataType& d) {
    // check the queue is not empty, if it isn't empty then d = data[read_idx] and advance read_idx by 1
    // returns false if queue is empty as pop failed
}
```

This implementation has a hidden performance issue related to cache coherency. Most CPUs use a cache coherency protocol (e.g., MESI/MOESI), which defines states for cache lines:

- Modified: Cacheline is only present in this core's cache and it is dirty (different to main memory)
- Exclusive: Cacheline is only present in this core's cache and it is clean
- Shared: Cache line is present in multiple cores' caches and is clean (roughly)
- Invalid: The cacheline is just invalid / unused

Consider a cacheline in modified state.
If another core wants to read that data and goes to main memory it will get an outdated copy.
This is obviously an issue, so we need to transition the cachelines state from modified to shared so the other core can read it.
In shared state the data should be "clean" so it should get written back to memory.
In a textbook MESI the cacheline needs to be written back to main memory so it can enter exclusive state from modified.
Then it can freely transition to shared, but a write back to main memory may take 150 cycles which is an eternity.

Realistically it may just use a cache to cache transfer which is faster like in MOESI.
The owner is responsible for writing it back to memory and everyone else is just viewing the modified data.
This requires cross‑core communication for cache‑to‑cache transfers, which is slow (tens of cycles) compared to an L1 access (~1–4 cycles) but fast compared to a main memory write back.

If a cache line is Shared and a core wants to modify it, that core must obtain exclusive ownership through an RFO(Request For Ownership aka Read For Ownership). The RFO requires the core to communicate with other cores, making them invalidate their copies of that cacheline and waiting for their reply. These transitions also take say ~30 cycles or so which can impact performance in tight loops.

Moving back to how this is actually relevant, say thread A is the pusher and thread B is the popper.
They are running in different physical cores.
For thread A to push it:

1. Reads write_idx and read_idx to check we can do a push
2. If we can do a push, do a write to the data vector and increment write_idx

For thread B to read it

1. Reads write_idx and read_idx to check if we can do a read
2. If we can do a read, read from the data vector and increment read_idx

Both threads are doing this concurrently which is leading to some cacheline ping-pong, importantly.
For example with A we move the write_idx cacheline to a modified state.
But B needs to read write_idx to check if the queues not empty so it can pop from it.
But write_idx's cacheline is in exclusive state so we need to move it into a shared state so our core can get a copy.
This causes cache‑line ping‑pong between cores, which hurts throughput. The same issue occurs for `read_idx`.
There is no perfect solution, but techniques exist to minimize inter‑core communication.

To avoid frequent cross‑core reads, keep cached copies of the remote index on a separate cache line. The writer compares `write_idx` with its cached `read_idx`; if `write_idx == cached_read_idx` then refresh the cached value from the consumer. This reduces costly transfers to occasional updates rather than on every push. The popper can use a cached `write_idx` symmetrically.

There was noticeable variability across runs even with pinned threads, so treat confidence intervals cautiously.

```
---Summary statistics for Classic ring buffer benchmark---
Sample mean cycles per test: 2.04386e+08
Confidence interval: 2.01442e+08-2.07331e+08
Sample standard deviation: 1.48374e+07
Tests used: 100
---Summary statistics for Caching ring buffer benchmark---
Sample mean cycles per test: 6.81954e+07
Confidence interval: 6.72367e+07-6.91541e+07
Sample standard deviation: 4.83165e+06
Tests used: 100
```

Overall I found about a ~3x speedup for a 10,000‑size buffer passing 10 million elements, and roughly ~2x for a 1,000,000‑element buffer passing 100 million elements.

```
---Summary statistics for Classic ring buffer benchmark---
Sample mean cycles per test: 2.16626e+09
Confidence interval: 2.16177e+09-2.17075e+09
Sample standard deviation: 2.26371e+07
Tests used: 100

---Summary statistics for Caching ring buffer benchmark---
Sample mean cycles per test: 1.00949e+09
Confidence interval: 9.95643e+08-1.02334e+09
Sample standard deviation: 6.97944e+07
Tests used: 100
```

Erik Rigtorp's article claimed larger improvements of 20x, and this shows actual gains can be very hardware‑dependent, so results vary by CPU and configuration. Nevertheless, these optimizations consistently produce substantial improvements.

As an aside alignas(64) does not always cut it since some computers have 128 byte cachelines.
This is not that extreme of an edge-case, modern Apple Silicon chips like the M series use 128 byte cache lines. The solution to this is through `std::hardware_destructive_interference_size` which is just the minimum offset between two objects needed to avoid false sharing. So aligning an object with that will let it sit in its own cache line.

# SSO

`std::string` differs from a C‑style `char[]` in that it's resizable and often manages a heap allocation (similar to `std::vector`). Allocating small strings on the heap can be costly, so many implementations use SSO (Small String Optimization): small strings are stored directly inside the `std::string` object (commonly in the space otherwise used for the pointer/capacity), avoiding a heap allocation.

Benchmark results (benchmark of small string sizes):

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

You can see the effect clearly: increasing the string size from 16 to 17 bytes causes a ~2.8× drop in performance due to it falling out of the SSO fast path.
This is a clear drop off between stack and heap allocation "modes", this is called SSO (small string optimization) as you avoid needing a heap allocation for small strings.
Note that by a string of size 16, I mean it holds 16 bytes total which includes the null terminator, not 16 characters + "hidden" 17th character null terminator.
So the dropoff starting at 16 shows the String uses at max a 16 byte char stack buffer inside it.
This used the libstdc++ standard library implementation where a string is 32 bytes total.
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

The union member only occupies the space of its largest member (the 16‑byte `stackbuf`). This layout reuses the 8‑byte `capacity` field alongside the 16‑byte buffer. It doesn't type‑pun both `data` and `capacity`, only `capacity`, so it "wastes" 8 bytes when using a heap‑allocated buffer (which needs two `size_t` values and a pointer). A benefit is the mode check is simple: if `data` points into `stackbuf`, the string is in SSO mode; otherwise it uses a heap buffer. This has benefits, when reading the string the Union does not need to internally check the "mode" it is in and perform any tricks, it just knows the data pointer is pointing at the start of the string, and the size field will be its length.

Some implementations (e.g., libc++) use different tricks, type‑punning multiple fields to increase in‑object storage (e.g., 23 characters in a 24‑byte object). A simplified illustration follows:

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

The implementation must distinguish stack vs. heap mode before being able to read the string unlike the libstdc++ one. One common technique encodes the mode in a pointer's low bits or uses a reserved bit in a size field. That may reduce the range of representable sizes in SSO mode (e.g., to 127) but is acceptable since SSO stores only small strings (≈22 characters plus a null terminator). In heap mode, capacities remain large (e.g., up to 2^63) which is more than enough. See a deeper explanation at https://joellaity.com/2020/01/31/string.html. This is more space efficient and could be faster if those extra characters are able to avoid needing a heap allocation. But now just accessing the string takes some overhead as you need to check what "mode" it is in first, it also increases complexity though it is just abstracted away.

# Sorting to help with branch prediction

CPU's for maximum speed use instruction pipelining for instruction level parallelism within a single core. A CPU core has many execution units which could all be working at the same time. Additionally you want to avoid the CPU stalling due to data dependencies, it may have to wait for B to finish before it can start C as C is dependent on B. Pipelining attempts to keep the whole processor busy by working on multiple instructions simultaneously that aren't dependent on each other. This is good to avoid stalling, but sometimes there are issues like branch instructions. These are essentially if statements, e.g. if (condition) do A else B. There is a clear dependency, depending on the condition different instructions may execute. The processor could stall but this is a loss in performance as it idles, so it uses a tool called branch predictions. Essentially the CPU makes an informed guess on whether the statement will be true or not. Say the CPU guesses it will be true, it will start executing A. If the CPU's guess is correct all is well and it has saved some time. If the CPU guesses wrong this speculative execution has lead to issues, it now needs to clear the instruction pipeline and start over. Clearing the instruction pipeline leads to a 15-20 cycle cost on an incorrect prediction. Interestingly for modern processors branch prediction has gotten more expensive because they use larger pipelines allowing for them to make better use of their high processing power. CPU's have a couple interesting tools to make good predictions. One of these is a Branch History Table which is basically as it sounds a history of whether that branch has been true or false in the past. If the Branch History Table shows that the recent branches have mostly evaluated to false, speculatively executing the instructions on a false evaluation is probably going to lead to less failed predictions.

This benchmark is an iteration through a list of random numbers in range a to b.
If that number is larger than (a+b)/2 a number gets incremented.
The compiler is stopped from making this loop branchless, so the cpu has to do branch predictions which are effectively random.
As such its basically going to flounder around failing every second prediction,
but by sorting this list before iteration the branch predictor will work great.
For the ~first half of the list it'll be smaller than the mean (approximately) while its larger than for the second half.
This makes branch prediction very easy, so sorting a list should result in improved performance.

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

As seen, this yields roughly a 90% performance improvement from a strange hardware‑level optimization.
Of course this is only if you forget about the sort itself.

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
500000 random indexes over the 100000 element array had good improvements from bitpacking.
Random indexing involves iterating over a vector of size t's which is also interesting,
this would use up far more memory than the actual array of elements here,
since this iteration is sequential its prefetched though.
The index vector (size_t) is sequential and benefits from hardware prefetching, so it doesn't consume as much cache as the random accesses themselves.

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

Found this interesting since a lot of people including me rag on bool vectors but its not always as clear cut as expected.
I still consider it a mistake like most though since the standard library should not use a strange heavily opinionated optimization.
Especially considering the way it stops this specialization from being a STL container.
Due to back compatibility reasons it can't be changed though.

# Mersenne Twister (MT19937) vs Xoroshiro family RNGs (and a SIMD accelerated one)

Mersenne Twister is often considered a standard RNG to go to but its quite slow and not good statistically.
An interesting fact is after observing just 624 iterations of MT19937 you can predict every future generated number.
Xoroshiro128+ is a RNG that is considered considerably faster with better statistical qualities.
Both were benchmarked for generating random integers.
Was expecting xoroshiro128+ to be substantially faster and it is, ~3.95x faster number generation is very welcome.
Xoroshiro also has better statistical properties.

```
---Summary statistics for Xoroshiro1Aka large enough alignment that this object will fit in its own cache line.28+ RNG---
Sample mean cycles per test: 2.41944e+06
Confidence interval: 2.4054e+06-2.43349e+06
Sample standard deviation: 123615
Tests used: 300

---Summary statistics for Mersenne twister RNG---
Sample mean cycles per test: 9.55954e+06
Confidence interval: 9.48389e+06-9.63519e+06
Sample standard deviation: 665787
Tests used: 300
```

I recently made a small library for 4 byte number generation (floats and uint32_t).
My implementation is a SIMD accelerated version of Xoroshiro64*,
it works by having multiple Xoroshiro64* lanes advancing in parallel.
This lets me have every lane advance simultaneously to create 16 random numbers at once.
I also have a second buffered version which has a buffer it refills using the batch api.
Its api is just a standard scalar get_next() so there's branching at the cost of less ops.
This benchmark is filling arrays with random 4 bit integers.

```
---Summary statistics for Mersenne twister RNG---
Sample mean cycles per test: 4.82333e+07
Confidence interval: 4.78512e+07-4.86153e+07
Sample standard deviation: 1.34424e+06
Tests used: 50

---Summary statistics for Xoroshiro64 RNG---
Sample mean cycles per test: 2.41813e+07
Confidence interval: 2.39376e+07-2.4425e+07
Sample standard deviation: 857640
Tests used: 50

---Summary statistics for Buffered Xoroshiro64 RNG---
Sample mean cycles per test: 1.84738e+07
Confidence interval: 1.81777e+07-1.87698e+07
Sample standard deviation: 1.04157e+06
Tests used: 50

---Summary statistics for SIMD Xoroshiro64 RNG---
Sample mean cycles per test: 3.74071e+06
Confidence interval: 3.71803e+06-3.7634e+06
Sample standard deviation: 79825.9
Tests used: 50
```

My simd accelerated version gets an incredible ~6.5x speedups over the standard xoroshiro64\* impl. The buffered version is ~1.3x faster which is still solid enough, shows the benefit of batching where possible.

# LIKELY / UNLIKELY Attributes and a Branch Prediction tangent

C++20 introduces the `[[likely]]` and `[[unlikely]]` attributes which are hints to the compiler about the expected probabilities of a branch. This is as strange as it sounds, you can mark an if statement as `[[unlilkely]]` and the compiler can make optimizations assume the other branch is more likely. This is purely for the compiler isn't related directly at least to how the CPU predicts branches for speculative executions.

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

This example uses `[[unlikely]]` and `[[likely]]` to intentionally mis‑hint the compiler.
If the compiler follows these hints, performance can suffer because it may optimize for the wrong path.
This is obviously just a hint to the compiler, definitely compiler specific and the compiler could just completely ignore it (this was done using GCC).
The tests compare three cases: correct hints (`LIKELY`), incorrect hints (`UNLIKELY`), and no hints (default behavior).

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

At a 70% probability for the first condition, short‑circuiting occurs less often and costs from failed branch predictions will increase.
Branch prediction costs dominate the short‑circuiting benefits: the branchless version is about 2.36× faster than the second‑place (`likely`) variant. The `unlikely` variant is only ~1.26× slower (not ~2.22×) because short‑circuiting advantages diminish while misprediction penalties grow.

# Costs and dangers of unaligned access

At the hardware level, data is organized into cache lines, which are typically 64 bytes long.
When the CPU fetches memory from RAM it fetches that memory by the whole cache line, the cache line is the smallest unit of data transferrable between RAM and CPU cache.
This is good because there is a spacial locality to data access, meaning getting some nearby data with what you want can help minimize future cache misses.
Too large of a cache line though and you pick up more irrelevant data which could displace actually useful stuff from the cpu's cache so there's a balance.
If you had an 8 byte piece of data, but it started at the end of one cache line and ended in the other when accessing it you would need to fetch both cache lines.
Additionally consider when loading the data into the register it would require manipulation of both cache lines.
Writing the data back would also need some messy bit manipulation to put say n starting bytes on the end of cache line A and the rest of the bytes on start of cache line B.
This obviously has performance impacts and is an example of unaligned access.
In the worst case a lot of older architectures don't support unaligned memory access which results in crashes, modernly though the issue is potential performance penalities.
An additional thing is the whole cache line is mapped to a single page, not spanning across two.
There is no guarantee that data will exist on one page if this piece of data is split across cache line boundaries though this would be fairly unlucky it could be pretty costly.
On older CPU's unaligned data access where data doesn't cross cache line boundaries (ie a uint64_t which starts not on a multiple of 8 but doesn't cross a multiple of 64) required additional operations apparently but on modern CPU's this isn't the case.
So for modern CPU's the effect of unaligned access I would expect to only be observable when crossing cache line boundaries which is what this benchmark is.
This benchmark measures time taken for a large number of uint64_t writes at a given byte offset to an array.
The array is 64 byte aligned so a byte offset will correspond 1:1 to a cache line offset.

```
---Summary statistics for aligned array write, byte offset 0---
Sample mean cycles per test: 194159
Confidence interval: 188010-200307
Sample standard deviation: 69974.8
Tests used: 500
---Summary statistics for unaligned array write, byte offset 7---
Sample mean cycles per test: 194014 // unaligned access but same speed as expected since it doesn't cross a cacheline boundary
Confidence interval: 187466-200563
Sample standard deviation: 74527.6
Tests used: 500
---Summary statistics for aligned array write, byte offset 8---
Sample mean cycles per test: 199984
Confidence interval: 189751-210216
Sample standard deviation: 116454
Tests used: 500
---Summary statistics for unaligned array write, byte offset 13---
Sample mean cycles per test: 196464 // there's some variations but can see the confidence intervals basically overlap
Confidence interval: 188339-204590
Sample standard deviation: 92481.5
Tests used: 500
---Summary statistics for aligned array write, byte offset 16---
Sample mean cycles per test: 197986
Confidence interval: 189263-206709
Sample standard deviation: 99273.9
Tests used: 500
---Summary statistics for aligned array write, byte offset 32---
Sample mean cycles per test: 197281 // can see accesses so far time taken has no relation with byte offset
Confidence interval: 188969-205593
Sample standard deviation: 94598.2
Tests used: 500
---Summary statistics for unaligned array write, byte offset 55---
Sample mean cycles per test: 194436
Confidence interval: 188184-200689
Sample standard deviation: 71160.4
Tests used: 500
---Summary statistics for aligned array write, byte offset 56---
Sample mean cycles per test: 192714
Confidence interval: 190027-195401
Sample standard deviation: 30583.8
Tests used: 500
---Summary statistics for unaligned and crossing cacheline boundaries array write, byte offset 57---
Sample mean cycles per test: 762896
Confidence interval: 759341-766451 // and all of a sudden performance tanks where its crossing cacheline boundaries
Sample standard deviation: 40454.4
Tests used: 500
---Summary statistics for unaligned and crossing cacheline boundaries array write, byte offset 58---
Sample mean cycles per test: 763787
Confidence interval: 758914-768660
Sample standard deviation: 55457.4
Tests used: 500
---Summary statistics for unaligned and crossing cacheline boundaries array write, byte offset 59---
Sample mean cycles per test: 768911
Confidence interval: 755482-782340
Sample standard deviation: 152838
Tests used: 500
---Summary statistics for unaligned and crossing cacheline boundaries array write, byte offset 63---
Sample mean cycles per test: 784827 // tanks for all of them the same amount
Confidence interval: 756972-812683
Sample standard deviation: 317024
Tests used: 500
```

Very clear benchmark, crossing cache line boundaries leads to an awful ~3.95x longer time taken to perform the write.
For unaligned access which doesn't cross cache line boundaries there's no performance impacts (at least on my modern CPU).
Unaligned access is also undefined behaviour in C++ so do try to avoid this.
To avoid unaligned accesses, the compiler gets around this by aligning your data through creating padding.
That way accessing the data will be aligned as the data itself is aligned.

# Padding to avoid unaligned access, and reorganizing a struct for less memory usage

To align data the way I think of it is we need to find memory addresses it can start at that don't cross cache lines.
A cache line is typically 64 bytes, though some chips implement different size cache lines but they are all of 2^n in size.
If you wanted to find if a memory address was "safe" to place 4 bytes of data without it crossing cache lines, you could check if that address was divisible by 4.
[0:63] wouldn't allow data to start at 61-63 as some bytes would cross a cache line.
60 which is divisible by 4 would be safe as it would take up [60, 61, 62, 63].
On 64 bit linux systems a long double is typically on 10 bytes of size.
A 4 byte alignment obviously wouldn't work since starting at address 60 would lead to it overflowing.
A 10 byte alignment would also mark 60 as fine for it to start which it isn't.
Rather you need the smallest factor of 64 which is as large as your piece of data for proper alignment.
A good property of c = 2^n, is its factors are 2^x for 0 <= int x <= n.
So a 10 byte long double would be "wasting" 6 bytes of space as it needs to be properly 16 byte aligned.
This is actually the case on linux.

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

Another interesting thing is empty base optimization fails if the first non static member (whom address it overlaps) is the same type, or has a base which is the same type as that member. This is because you can't have two objects of the same type at the same address (even if both have no unique address).

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

As an aside additionally using no unique address on a member which isn't empty (like an int) will just be ignored by the compiler as it does space.

# Zero Cost Compile Time Storage Specialization

This technique I thought of makes some very interesting use of `[[no_unique_address]]` paired with conditional types, variadic templates and requirements. Read the above statement on how EBO and no unique address works then the rest should become evident. Recently I created a SIMD accelerated RNG that would generate a vector register sized array of random numbers (say 64 bytes but generally depends on hardware). Most users though only need to request say a 4 byte integer, float or double though, so to facilitate this I created a wrapper that contains buffers of elements, which it dispenses to consumers and refills once used. But the space cost of the wrapper is substantial, holding an array 64 bytes for ints, a second array for floats and a third for doubles is expensive. While I could have the wrapper use some bit manipulation to convert the random int into a `[0, 1]` float, my SIMD accelerated RNG uses vector instructions to do this in batches so it would lose in performance. Another alternative would be using one array which holds an internal state to check if its holding ints/doubles/or floats but this would lead to extra branching on every access hurting performance. Additionally a large number of callers when making the wrapper will likely only want to say generate floats/or ints so they would pay a space penalty for an abstraction that should be zero cost. My solution was having callers template my buffered rng with a variadic template of the exact types they wanted to generate. This lead to two issues, I would need a way to:

- Conditionally enable member functions based on membership of their type generated in the variadic template
- Conditionally enable the members of a class based on its variadic template

The first issue is quite forward using SFINAE like std::enable_if or more modernly with requirements (part of C++ 20's Concepts).

```
// To check membership of a type in a variadic template we can use fold expressions
template <typename Term, typename... Set>
concept OneOf = (std::same_as<Term, Set> || ...);

// Capabilities are the types our RNG can produce, we need to limit these to what we actually can produce. We can do this with a fold expression using our concept.
template <typename... Capabilities>
    requires(OneOf<Capabilities, uint32_t, int32_t, float, double> && ...)
class BufferedXoroshiroRNG {

    // etc an example get_float method check
    float get_float()
        // Only enabled if float is one of Capabilities
        requires OneOf<float, Capabilities...>
    {
        if (float_idx == XoroshiroRNG::BATCH_SIZE) {
            float_idx = 0;
            float_buffer = rng.get_batch_floats();
        }

        return float_buffer[float_idx++];
    }

    // ... continues
}
```

This is fairly straightforward but things become difficult when it comes to only creating the float_buffer and float_idx members when float is one of capabilities. An interesting solution I thought of involves conditional types and `[[no_unique_address]]`.

```
// Don't override this tag
template <auto Tag = [] {}>
class Empty {};

// We want to be able to conditionally "disable" a member if its not templated with it
// FLOAT_ARR is a type alias, if float is in Capabilities it will be an array, else an Empty<>
using FLOAT_ARR = std::conditional_t<OneOf<float, Capabilities...>, std::array<float, XoroshiroRNG::BATCH_SIZE>, Empty<>>;
using FLOAT_INDEX = std::conditional_t<OneOf<float, Capabilities...>, uint8_t, Empty<>>;

[[no_unique_address]] UINT_ARR uint32_t_buffer;
[[no_unique_address]] UINT_INDEX uint32_t_idx{};
```

Even if UINT_ARR is a class with no members (0 bytes size), it is a member of our class so it is guaranteed a unique memory address according to the standard so it will take 1 byte. A common way to avoid wasted space is empty base optimization, which essentially says an empty base class isn't guaranteed a unique memory address. This is quite useful, std::vector for example internally inherits from a memory allocator. The default allocator is stateless but due to EBO it doesn't need a unique memory address so no unnecessary memory is used. But this approach isn't suitable here since our buffer and the idx is a member. C++ 20 however introduces the attribute `[[no_unique_address]]`, which allows you to indicate to the compiler that a non static member does not need a unique address. This allows it to overlap a 0 byte non static member with the address of the first non static member. If you use `[[no_unique_address]]` on an object that takes space however (e.g. our buffer or idx into it) the compiler will just ignore the attribute. As such we have now conditionally enabled members of our class based on its template at zero excess space cost.

But there's one additional trick to this. EBO and `[[no_unique_address]]` while they allow overlapping empty objects with the first non static member, you cannot have 2 distinct sub-objects of the same type at the same memory address.

```
struct Empty{};

struct IncorrectBaseOptimization : Empty {
    Empty e;
    std::byte b;
};
static_assert(sizeof(IncorrectBaseOptimization) == 3);
```

Here's an interesting example, EBO fails to apply because the first non static member is of type Empty, having the base class Empty share the same address would violate this. As such it gets tacked onto the end and takes up an additional byte of space so this struct takes 3 bytes. My first implementation of my storage specialization ran into this problem as I used a `using Empty = decltype([]{});` to get an empty class. My issue was with this.

```
using FLOAT_ARR = std::conditional_t<OneOf<float, Capabilities...>, std::array<float, XoroshiroRNG::BATCH_SIZE>, Empty>;
using FLOAT_INDEX = std::conditional_t<OneOf<float, Capabilities...>, uint8_t, Empty>;
[[no_unique_address]] UINT_ARR uint32_t_buffer;
[[no_unique_address]] UINT_INDEX uint32_t_idx{};
```

If float is not one of capabilities, this means both uint32_t buffer and uint32_t_idx are both of the same Empty class.
So while the first Empty can be overlaid with the first non static data member, overlaying the second Empty would have two sub-objects of the same type at the same address. In my implementation I observed using `offsetof` was the first Empty object would be at 0 offset, but every other Empty object after had a unique memory address because they couldn't share a memory address.

A simple solution would be giving Empty an int template to generate a new class for each empty member. Replacing each Empty with `decltype([]{})` in my type aliases would also work but these are verbose and semantically aesthetic. My solution was a trick I learnt from stateful metaprogramming to force the compiler to generate a new class from the template each time it is called.

```
template <auto Tag = [] {}>
class Empty {};

static_assert(!std::same_as<Empty<>, Empty<>>); // This passes
```

Declaring a type of Empty<> results in Empty's Tag template being assigned to the default structural type which is a lambda. The key to this is that with each declaration of Empty<> the compiler generates a new lambda which Tag is assigned to. Lambda is a closure type, and each generated closure type is unique to any other type. This means both Empty's are specialized with completely different types despite the syntax being identical. So I changed to using this need implementation of Empty, and now each empty buffer or index has a completely different type so it is valid for all of them to share the same memory address.

# Exceptions to the as-if rule

Compilers provide a variety of optimizations through rewriting, reordering and elimination statements/expressions.
This is good as it allows programmers to create readable code without sacrificing performance.
Generally compilers follow the as-if rule which essentially allows the compiler to optimize however it wants if it doesn't change observable behaviour.
This is important because you want to be able to trust that regardless of the compilers optimization level the program runs the exact same.
However there are a few reasonable exceptions to as-if optimization in C++, these are:

- Return Value Optimization (RVO) / Named Return Value Optimization (NRVO) / copy elision
- Allocation elision (the compiler is free to optimize out heap allocation even if it has observable side effects surprisingly)
- Not really counted but undefined behaviour (but here you don't have any guarantees on what the program does anyways)

Imagine a scenario where construction / destruction of class A has observable behaviour

```
class A {
   public:
    A() {
        static size_t constructions{};
        std::cout << "Constructed N: " << ++constructions << '\n';
    }

    ~A() {
        static size_t deletions{};
        std::cout << "Deleted N: " << ++deletions << '\n';
    }
};

class AFactory {
   public:
    A create() {
        return A{};
    }
};

int main() {
    AFactory factory{};
    A a = factory.create();
}
```

Naively thinking through this code with something like a debugger, creating/destruction class A will lead to a print.
For `A a = factory.create();` since factory.create() is returning an rvalue we will call the move constructor to create this instance of A.
So factory.create() is going to run first, which is just `return A{}`.
This is going to construct an instance of A, then the move constructor will be called on a using this instance.
Then the rvalue will die and be destructed, then the stack will unwind and a will be destructed.
So we could expect a construct construct delete delete to be printed.
But this would obviously lead to unneeded overhead, when we could instead just have factory.create() construct the object inside a's address.
This is called RVO (return value optimization) and its guaranteed to occur when returning a prvalue (pure right value).
This is even if it leads to different observable behaviour so its an exception to the as-if rule.
Since this elides a copy we only see construct delete printed, it completely elides one of the constructions/deletions.
Since RVO is guaranteed this will happen even with no optimization so behaviour can be expected, but this isn't the case for NRVO.
NRVO is for lvalues, e.g.

```
class AFactory {
   public:
    A create() {
        A a{};
        // etc
        return a; // A is an lvalue not a prvalue

        // note it must be a prvalue, not just an rvalue. This makes sense, consider std::move which is basically an rvalue cast.
        // return std::move(a); // would return an xvalue which is a type of rvalue, but its obviously quite the different case.
    }
};
```

This is a named value since its an lvalue not just an rvalue temporary. For NRVO, copy elision can happen but its not guaranteed.
This means behaviour can differ depending on optimization level, this could lead to 2 construction and 2 deletions or just 1 of each.
This makes sense since you shouldn't really be depending on observable behaviour of construction/deletion.

Allocation Elision is less well known but interesting.
It essentially says heap optimization can be optimized out by the compiler, even if that heap allocation has observable behaviour.
This is good since heap allocation is expensive so if the compiler can find a way to stack allocate it thats better.
This can lead to some interesting behaviour if you override new though.

```
// As big c fans we overload new and delete with malloc and free
void* operator new(size_t size) {
    std::cout << "malloc called" << '\n';
    void* ptr = malloc(size);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    std::cout << "free called" << '\n';
    // We also introduce a bug
    free(ptr);
}

int main() {
    {
        int* ptr = new int(5);
        delete ptr;
    }

    return 0;
}
```

Here we introduce a bug. While free deallocates memory, new both calls the destructor and deallocates memory.
This means if we use delete on an object, its destructor isn't getting called here which could lead to memory leaks.
For this scenario lets just forget about that. We overload our new / delete with malloc and free, printing when they are called.

```
int* ptr = new int(5);
delete ptr;
```

Heap allocation obviously has no purpose but since allocation is observable so you would think it doesn't get optimized out.
Or at least the print function still has to get called, but maybe it optimizes out internal malloc and free ops.
Compiling it with GCC and no optimizations it prints that malloc and free gets called.
However compiling with just -O1 leads to no prints being made.
This is since the compiler is free to elide allocations even if that allocation has observable side effects.

# Compile time string hashing

This is a method I thought up of for aesthetic compile time string hash. There's some pretty easy methods to do this like having the hash function accept a string literal, i.e. `hash_string<"str">();` and its clear from there. This would stop it from being called with runtime values but its kindof ugly, it also needs a second function for runtime strings. Could be prone to misuse if someone tries to do `hash_string("comp time str")` and forgets to template it also. This is a method where you can do a compile time hash cleanly, everything abstracted away behind the same interface for string literals or std::strings. See below:

```
std::string a = "hello world";
int hash = hash_string(a); // returns runtime constant

// But calling it with a string literal directly does a compile time hash
constexpr int comp_hash = hash_string("hello world");
```

Since a string literal is just a compile time char array, so we can overload a function to accept either a char array or a string_view /or const std::string& but string_view is more modern. This can be done by having an overload that accepts a c style array by reference.

```
template <size_t N>
int consteval string_hash(const char (&arr)[N]) {
    long long hash{};

    // A generic hash function could be whatever
    long long p = 31, m = 1e9 + 7;
    long long p_pow = 1;  // Store p ^ i
    // N - 1 to remove null terminator
    for (size_t idx{}; idx < N - 1; ++idx) {
        hash = (hash + (arr[idx] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }

    return hash;
}

int string_hash(std::string_view s) {
    long long hash{};
    // etc runtime version of same hash function
    hash{};
}
```

This would be another fun way but worse way to do it.

```
// have hash function delegate to an impl
// it makes the call `return hash_impl(arr, std::make_index_sequence<N - 1>());`
template <size_t N, size_t... IDXS>
int consteval hash_impl(const char (&arr)[N], std::index_sequence<IDXS...>) {
    long long hash{};

    long long p = 31, m = 1e9 + 7;
    long long p_pow = 1;  // Store p ^ i

    auto update = [&](size_t idx) -> void {
        hash = (hash + (arr[idx] - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    };

    (update(IDXS), ...);
    return hash;
}
```

# CRTP

CRTP or curiously recurring template pattern is a curiously recurring pattern using templates.The concept is fairly strange.

```
template <typename Derived>
class Base{};

// See the recursion as A has a base of Base specialized by A.
class A : public Base<A> {};

// Can also have another class that uses this with the same base
class Derived : public Base<Derived> {};
```

Essentially `Base` is instantiated on `Derived`, and `Derivied` inherits from `Base<Derived>`, which has `Derived` inherit from `Base<Derived>` and yada yada. So if you wanted to define Base you may think this wouldn't compile due to this recursion. Why it works is an interesting feature of how template instantiation works with incomplete types.

```
template <typename T>
struct Foo {
    void print() { std::cout << "Hello World\n"; }
};

// We have omitted a definition
// This means we have an incomplete type
struct IncompleteType;

int main() {
    Foo<IncompleteType> a{}; // This is completely fine
    a.print(); // Compiles and prints fine
}
```

Specializing Foo with T doesn't require T to be a complete type, it just needs to know that it is a type. In our CRTP example Derived is used in its own definition, which means it hasn't been fully defined yet, so Derived is an incomplete type like Undefined. So it goes like this.

- Start generating a definition for Derived
- Derived inherits from `Base<Derived>`
- Requires generating an instantiation of Base specialized by Derived
- Derived has not been instantiated so it is an incomplete type
- This is fine though, we can instantiate Base with an incomplete type
- `Base<Derived>` is then simply created
- Continue defining A

Essentially `Base<Derived>` can be instantiated with an incomplete `Derived`, so instantiation of the Base specialization doesn't require the definition of Derived which would lead to a recursive loop.

CRTP has a couple benefits, one of them is keeping the hierarchial benefitits and code reusability of inheritance but with all calls getting resolved at compile time so there is no dynamic dispatch. As such its not true "inheritance" as you can't have say a `std::vector<X<A and B?>>` obviously since they are different types. But you can have something interesting like this.

```
template <typename T>
class Writer {
   public:
    template <size_t N>
    void write(const char (&arr)[N]) {
        // Static cast is fully compile time, we know we can cast this object into a T,
        // as this object is a T inheriting from Writier<T>
        static_cast<T*>(this)->writeImpl(arr);
    }

    void shared_behaviour() {
        // We can have some common behaviour among children also since its inheritance obv
    }
};

// Now we can make some writers that use CRTP
class CoutWriter : public Writer<CoutWriter> {
   public:
    template <size_t N>
    void writeImpl(const char (&arr)[N]) {
        std::cout << arr << '\n';
    }
};

// And another one which does its writeImpl using printf
class PrintfWriter : public Writer<PrintfWriter> {
   public:
    template <size_t N>
    void writeImpl(const char (&arr)[N]) {
        printf("%s\n", arr);
    }
};

// Now we can make an interesting function like this
template <typename T>
void printStuff(Writer<T>& w) {
    // We know that all Writer<T> will have a write method which delegates to T's writeImpl.
    // However this is compile time "polymorphism" there is no costly dynamic dispatch.
    // Additionally the compiler can optimise calls better by inlining and etc as it uses compile time dispatch.
    w.write("Stuff");
}

int main() {
    CoutWriter coutWriter{};
    PrintfWriter printfWriter{};

    // It looks like polymorphism but its all resolved at compile time nicely
    printStuff(coutWriter);
    printStuff(printfWriter);
}
```

There would be alternative approaches also to achieve something similar.
You could use a ducktyping esque implementation thats still statically resolved like this.

```
// Now we can make an interesting function like this
template <typename T>
requires requires(T t) { // Require statements like this have pretty ugly syntax with the nested requires sadly
    t.write("Test String"); // Here we just check it has a method write which accepts at least a string literal
}
void printStuff(T w) {
    w.write("Stuff");
}
```

The benefit behind CRTP though is that you get the structure of inheritance, so you can have a parent be an "interface" that delegates to its child at compile time through templates. I would consider it clearer, and if a class inherits from it it would inforce at compile time that the class has the implementations the base class delegates to it. The other major benefit which this doesn't get is that it provides the ability to reuse code. This is an interesting example of CRTP where class A inheriting from `Singleton<A>` will basically turn it into a singleton.

```
template <typename T>
class Singleton {
   public:
    static T& getInstance() {
        // Use static instance inside method, lazily evaluated so avoids static
        // initialization fiasco issues
        static T instance;
        return instance;
    }

   protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

// Through CRTP we can turn A into a singleton (with mini caviats)
class A : public Singleton<A> {
   public:
    double d{};
    // Note copy construction and etc wouldn't work because the parent has it deleted
   private:
    // We need Singleton to be a friend because the Singleton object manages the static instance
    // And the constructor needs to be private so noone else can create it
    friend Singleton;
    A() = default;
};

int main() {
    // :: uses parent Singleton<A> getInstance() which just returns A ref
    A& a = A::getInstance();
    a.d += 2.5; // We can access class A's members

    A& still_a = A::getInstance();
    std::cout << still_a.d << '\n';
    // out prints 2.5

    return 0;
}
```

# Compile Time State

Compile time state can be achieved through stateful metaprogramming, and its an effective way to render your program ifndr. Stateful metaprogramming was not an intentionally created feature of the language, rather it was discovered by combining together multiple tricky features. It is arcane and should probably be ill formed.

```
constexpr bool N = check_counted<0>();
Writer<0> a;
constexpr bool M = check_counted<0>();
static_assert(N != M); // And this assert passes!
```

We have the same consteval function return a different value. The way this works involve a few things, but the core idea is that state is stored inside whether a function has a definition.

```
template <unsigned NextVal, auto Tag = []() {}>
consteval bool check_counted() {
    constexpr bool counted_past_value = requires(Reader<NextVal> r) {
        counted_flag(r);  // will be true if this has a definition
    };

    return counted_past_value;
}
```

This is a more modern way to write it using c++23. Requires statements are commonly used with templates as a more readible version of SFINAE. It effectively will return false if the expression in it is ill formed or it voilates any of the constraints in it. We use it to check if its valid to call` counted_flag(r)`, if `counted_flag(r)` has no definition then it will return falsy, and if it has a definition it will return truthy. But there's a subtle issue with just this, the compiler sees check_counted() is a pure consteval function, so it will probably just evaluate it the first time its called then reuse that result. The solution to force the compiler to reevaluate it through calling a different function each time.

```
template <unsigned NextVal, unsigned Tag>
consteval bool check_counted() {
    // stuff
}
```

Now we can call it for the same NextVal, but specify a different Tag, where the Tag does nothing but it forces the compiler to instantiate it again as its templated different. This is kindof messy though having the caller do this, so a very clean way to do it is through having the structural type being an unnamable type.

```
template <unsigned NextVal, auto Tag = []() {}>
```

Each call to it with an integer, will have the compiler evaluate the Tag default argument by creating a new lambda. Since its a different lambda being made each time these are all unique structural types. This will result in the compiler needing to instantiate `check_counted<0>()` every time its called.

So now we need a way to declare, and later define the `counted_flag(r)` function as whether its got a definition or not is our state. This can be done through friend injection.

```
template <unsigned N>
struct Reader {
    friend auto counted_flag(Reader<N>);
};

template <unsigned N>
struct Writer {
    // defines the counted_flag for the unsigned N
    friend auto counted_flag(Reader<N>) {}
};
```

Consider that the scope of a friend function is the innermost enclosing scope, in this case global scope. So by instantiataing a `Reader<0>` we create a declaration for a `counted_flag(Reader<0>)` function in global scope. The friend function while in global scope it is only accessible through argument dependent lookup.

A quick explalnation is basically with for example `endl(std::cout)`, endl has not had its scope resolved to the std namespace so you wouldn't find the function in the global namesapce. But the compiler uses ADL (argument dependent lookup), where since an argument is in the std namespace, the compiler will additionally lookup functions in the std namespace. As such this compiles fine. We do something similar.

```
<unsigned N>
struct Reader {
    friend auto counted_flag(Reader<N>);
};
```

`counted_flag` has to accept an argument of the readers own type so that `counted_flag` can be found through argument dependent lookup by the requires statement below.

```
constexpr bool counted_past_value = requires(Reader<NextVal> r) {
    counted_flag(r);  // will be true if this has a definition
};
```

And as such all together we get this.

```
template <unsigned N>
struct Reader {
    friend auto counted_flag(Reader<N>);
};

template <unsigned N>
struct Writer {
    // defines the counted_flag for the unsigned N
    friend auto counted_flag(Reader<N>) {}
};

template <unsigned NextVal, auto Tag = []() {}>
consteval bool check_counted() {
    constexpr bool counted_past_value = requires(Reader<NextVal> r) {
        counted_flag(r);  // will be true if this has a definition
    };

    return counted_past_value;
}

constexpr bool N = check_counted<0>();
Writer<0> a;
constexpr bool O = check_counted<0>();
static_assert(N != O);

constexpr bool P = check_counted<1>();
static_assert(!P);
```

WIP

# plans

- Benchmarks for different types of containers, eg vector vs sparse set vs linked list vs colony/hive
- proper statistics with anova, levene test, residual normality checking and other stuff
- prefetcher stuff with contiguous memory access graphic latency vs different working sizes, compare against linked list pointer chasing
- simd/vectorisation falls into SOA kinda
- false sharing messing with concurrency
- maybe LTO
- different dynamic dispatch methods
- different binary search memory layouts
- simd binary search
