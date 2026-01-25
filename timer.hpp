#pragma once

#include <x86intrin.h>
#include <cstdint>
#include <cpuid.h>
#include <stdexcept>
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
        if (isTiming) throw std::runtime_error("Can't have multiple timers going at once");

        isTiming = true;
        unsigned int aux;
        // Serialize to avoid out-of-order effects
        __cpuid(0, aux, aux, aux, aux);
        startTime = __rdtsc();
    }

    // Ends the timer returning the number of clock cycles the timer lasted
    [[nodiscard]] uint64_t endTimer() {
        unsigned int aux;
        // rdtscp is partially serializing; follow with cpuid
        uint64_t endTime{ __rdtscp(&aux) };
        __cpuid(0, aux, aux, aux, aux);

        if (!isTiming) throw std::runtime_error("Can't end a timer that hasn't started");
        isTiming = false; 

        // technically doesn't "detect it" but it'd take 100 years+ to do more than 2^64 cycles
        // shouldn't ever trigger since turning on the laptop restarts it at 0 but feel like it should be there
        if (endTime < startTime) throw std::runtime_error("64 bit unsigned integer overflow somehow?");

        return endTime - startTime;
    }
    
private:
    Timer(); // private construction
    bool isTiming{ false };
    uint64_t startTime;
};