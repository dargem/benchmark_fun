#pragma once
#include <atomic>
#include <cassert>
#include <string_view>
#include <vector>

namespace benchmarks {
struct Slot {
    alignas(64) std::atomic<size_t> sequence{0};
    int value;
};

class VyukovAtomicQueue {
   public:
    VyukovAtomicQueue(size_t size) : slots(size), mask(size - 1) {
        assert((size & mask) == 0);  // must be power of two for mask trick
        // Going to initialize each slot with its index for the sequence
        for (size_t i = 0; i < size; ++i) slots[i].sequence.store(i, std::memory_order_relaxed);
    }

    bool push(int val) {
        size_t pos = write_idx.load(std::memory_order_relaxed);
        while (true) {
            // Bit manipulation mod trick to cycle through array
            Slot& slot = slots[pos & mask];
            size_t seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                // If our slots sequence is equal to position we can increment write_idx.
                // Slot is ready so we increment pos (note with no wrap around)
                if (write_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
                // We have loaded pos form write_idx at the start. Another thread may have seized
                // this slot since though. If they seize it they would've updated write_idx so this
                // CAS instruction would fail. This CAS instruction if it fails updates pos with the
                // write_idx's value.
            } else if (diff < 0) {
                // Our seq < pos. In this case we may have wrapped around our slots and gotten back
                // to low sequences. If these slots were read from they would've been incremented by
                // a loop to fix this so the queue is completely fill.
                return false;
            } else {
                // We have lost thee CAS race before we started. We looked at seq and someone else
                // has both taken the write_idx and claimed the slot updating the seq. We now reload
                // our pos.
                pos = write_idx.load(std::memory_order_relaxed);
            }
        }
        // Now our write_idx has been updated and we are the only claimers to this slot. But we need
        // to make sure another thread doesn't read from this write_idx since while we've increased
        // it we haven't written to it. We communicate this has been written to by changing sequence
        // to pos + 1, where pos is our reserved slot. Initially to write to it pos was == seq. So
        // when the Nth slot has a seq of N+1 we know we have written to it.
        slots[pos & mask].value = val;
        slots[pos & mask].sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(int& val) {
        size_t pos = read_idx.load(std::memory_order_relaxed);
        while (true) {
            Slot& slot = slots[pos & mask];
            size_t seq = slot.sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);
            // If we've written to it already we would expect seq - (pos + 1) to equal 0. We can't
            // use write_idx as a tool since we may have incremented write_idx (to reserve it) while
            // having not yet actually written to it.

            if (diff == 0) {
                // Do a CAS instruction, if another thread hasn't read while we were doing this we
                // can make progress.
                if (read_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
                return false;
                // seq == pos initially which means this hasn't been written to
            } else {
                pos = read_idx.load(std::memory_order_relaxed);
            }
        }
        // We've incremented write_idx which means we can return this now
        val = slots[pos & mask].value;
        // If been read we store index for that slot + mask + 1 which is just slot + n_slots. So
        // this just increases leading bit and & mask will get pos back. This makes sense to get
        // back to this slot we will need 1 more full loop. Note we don't reset pos we use & to do
        // wrap arounds.
        slots[pos & mask].sequence.store(pos + mask + 1, std::memory_order_release);
        return true;
    }

    static constexpr std::string_view NAME = "Vyukov MPMC";

    void reset() {}

   private:
    std::vector<Slot> slots;
    size_t mask;
    alignas(64) std::atomic<size_t> write_idx{0};
    alignas(64) std::atomic<size_t> read_idx{0};
};

}  // namespace benchmarks