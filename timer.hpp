#pragma once

#include <x86intrin.h>
#include <cstdint>
#include <cassert>

// A singleton because multiple timers shouldn't exist and singletons are fun
// Header only to allow inlining of the whole class
class Timer
{
public:
    // Get a reference to a timer object
    static Timer& getInstance() {
        static Timer timer; // lazy construct a timer
        return timer;
    };

    void operator=(const Timer&) = delete; // delete copy assignment
    Timer(const Timer&) = delete; // delete copy constructor
    void operator=(Timer&&) = delete; // delete move assignment
    Timer(Timer&&) = delete; // delete move constructor

    // Starts the timer, a timer can not be started again when its already going
    void startTimer() {
        assert(!isTiming && "Can't have multiple timers going at once");

        isTiming = true;
        // load fence ensures all previous instructions completed before 
        // it starts the timer 
        _mm_lfence();
        startTime = __rdtsc();
    }

    // Ends the timer returning the number of clock cycles the timer lasted
    uint64_t endTimer() {
        // used by __rdtscp to store some auxiliary info
        unsigned int aux;
        // reads the processor's time stamp counter (in cpu cycles)
        uint64_t endTime{ __rdtscp(&aux) };
        // load fence ensures all previous instruction completed
        // so that this only executes when everything its benchmarking is done
        _mm_lfence();

        assert(isTiming && "Can't end a timer that hasn't started");

        // technically doesn't "detect it" but it'd take 100 years+ to do more than 2^64 cycles
        assert(endTime > startTime && "64 bit unsigned integer overflow, time to get a lottery ticket");

        return endTime - startTime;
    }
    
private:
    Timer(); // private construction
    bool isTiming{ false };
    uint64_t startTime;
};