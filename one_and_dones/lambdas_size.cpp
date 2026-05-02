#include <cstddef>
#include <iostream>

using A = decltype([]() {});

struct ByteStruct {
    std::byte b;
};

static_assert(sizeof(ByteStruct) == 1);

struct Empty {};

static_assert(sizeof(Empty) == 1);

struct BaseOptimization : Empty {
    std::byte b;
};

static_assert(sizeof(BaseOptimization) == 1);

struct NoBaseClass {
    std::byte c;
    Empty e;
};

static_assert(sizeof(NoBaseClass) == 2);

struct MemberOptimization {
    std::byte c;
    [[no_unique_address]] Empty e;
};

static_assert(sizeof(MemberOptimization) == 1);

struct NormalLambdaStruct {
    std::byte c;
    A a;
};

struct NoUniqueAddressLambda {
    std::byte c;
    [[no_unique_address]] A a;
};

int main() {
    std::cout << "Size of empty class: " << sizeof(A) << '\n';
    std::cout << "Size of empty class inheriting an empty class: " << sizeof(A) << '\n';
    std::cout << "Size of member base class: " << sizeof(NoBaseClass) << '\n';
    std::cout << "Size with no unique address optimization: " << sizeof(MemberOptimization) << '\n';

    std::cout << "Size of empty lambda: " << sizeof(A) << '\n';
    std::cout << "Size of normal lambda struct: " << sizeof(NormalLambdaStruct) << '\n';
    std::cout << "Size with no unique address lambda: " << sizeof(NoUniqueAddressLambda) << '\n';
    return 0;
}