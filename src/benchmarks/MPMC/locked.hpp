#pragma once

#include <mutex>
#include <queue>
#include <string_view>

namespace benchmarks {

class MutexQueue {
   public:
    MutexQueue(size_t size) {}  // std::queue is dynamically resized so ignore

    bool push(int val) {
        std::lock_guard<std::mutex> acquire(m);
        q.push(val);
        return true;  // std::queue push cannot fail
    }

    bool pop(int& inp) {
        std::lock_guard<std::mutex> acquire(m);
        if (q.empty()) return false;

        inp = q.front();
        q.pop();
        return true;
    }

    void reset() {}

    static constexpr std::string_view NAME = "Mutex MPMC Queue";

   private:
    std::mutex m;
    std::queue<int> q;
};

}  // namespace benchmarks