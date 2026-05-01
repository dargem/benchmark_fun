#include <cstddef>

namespace utils {

class Arena {
   public:
    Arena() : data(new std::byte[BYTE_SIZE]) {}

    // delete copy construction / assignment
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // delete move construction / assignment
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    ~Arena() { delete[] data; }

   private:
    static constexpr size_t BYTE_SIZE = 100000;
    std::byte* data;
};

}  // namespace utils