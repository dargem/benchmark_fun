#include <iostream>

// As big c fans we overload new and delete with malloc and free
void* operator new(size_t size) {
    std::cout << "malloc called" << '\n';
    void* ptr = malloc(size);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    std::cout << "free called" << '\n';
    free(ptr);
}

int main() {
    {
        // With standard optimization new and delete are called. The compiler can prove that this
        // could've been stack allocated, or realistically just completely optimized out. But
        // new/delete here is overloaded to have observable side effects so you would think it
        // couldn't. However it is part of the c++ standard that heap allocation can be elided, even
        // if it has observable behaviour. Because of this, standard compilation with no flags
        // results in it printing that malloc and free got called. With just -O1 though the compiler
        // optimizes it out and nothing gets printed.
        int* ptr = new int(5);
        delete ptr;
    }

    return 0;
}
