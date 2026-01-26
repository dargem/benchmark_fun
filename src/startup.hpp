#pragma once
#include <sched.h> 
#include <pthread.h>
#include <iostream>

namespace startup {

// pins the thread to a core, avoids core migration (moving active thread to another core)
void pin_thread(int core_id) {
    pthread_t this_thread = pthread_self();

    // Define and initialize a CPU set
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset); // Set the specific core

    // Set the affinity of the current thread
    int rc = pthread_setaffinity_np(this_thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error: pthread_setaffinity_np failed for core " << core_id << std::endl;
    } else {
        std::cout << "Thread successfully pinned to core " << core_id << std::endl;
    }
}

}