#include <cstddef>
#include <iostream>

using Lambda = decltype([]() {});
struct Empty {};

// In C++ every object is guaranteed a unique memory address during its lifetime. While construction
// of an object can be optimized away, if it exists it needs a unique memory address. Because of
// this even a struct with no members, or a functor with no captures will take up 1 byte
static_assert(sizeof(Empty) == 1);
static_assert(sizeof(Lambda) == 1);

struct BaseOptimization : public Empty {
    std::byte b;
};
// But if a BaseOptimization object is constructed, that object has taken up a memory address. The
// base object Empty itself is not required to have a unique memory address, it can share the first
// non static members address. In this case std::byte, removing std::byte would just share it with
// the one byte of padding needed to give BaseOptimization an address.
static_assert(sizeof(BaseOptimization) == 1);
static_assert(offsetof(BaseOptimization, b) == 0);

struct NoBaseOptimization {
    Empty e;
    std::byte b;
};
static_assert(sizeof(NoBaseOptimization) == 2);

struct FailedBaseOptimization : Empty {
    BaseOptimization b;
};
// Empty base optimization is prohibited if one of the empty base classes is also the type or the
// base of the type of the first non-static data member, so this would fail here as Empty base
// class, is a base of the type of the first non static member (BaseOptimization)
static_assert(sizeof(FailedBaseOptimization) == 2);

struct CorrectBaseOptimization : Empty {
    std::byte b;
    Empty e;
};
static_assert(sizeof(CorrectBaseOptimization) == 2);

struct IncorrectBaseOptimization : Empty {
    Empty e;
    std::byte b;
};
static_assert(sizeof(IncorrectBaseOptimization) == 3);

struct NoBaseClass {
    std::byte c;
    Empty e;
};
// This of course only works for empty base classes, their members have to have a unique memory
// address. So this is going to take up 2 bytes
static_assert(offsetof(NoBaseClass, c) != offsetof(NoBaseClass, e));
static_assert(sizeof(NoBaseClass) == 2);

// C++20 introduces the [[no_unique_address]] address which laxes the requirement for members to
// have a unique memory address. This means because Empty has no state, it can share the address of
// the first non static members memory address.
struct MemberOptimization {
    std::byte c;
    [[no_unique_address]] Empty e;
};

static_assert(sizeof(MemberOptimization) == 1);
static_assert(offsetof(MemberOptimization, e) == offsetof(MemberOptimization, e));

struct NormalLambdaStruct {
    std::byte c;
    Lambda a;
};

struct NoUniqueAddressLambda {
    std::byte c;
    [[no_unique_address]] Lambda a;
};

int main() {
    std::cout << "Size of empty class: " << sizeof(Lambda) << '\n';
    std::cout << "Size of empty class inheriting an empty class: " << sizeof(Lambda) << '\n';
    std::cout << "Size of member base class: " << sizeof(NoBaseClass) << '\n';
    std::cout << "Size with no unique address optimization: " << sizeof(MemberOptimization) << '\n';

    std::cout << "Size of empty lambda: " << sizeof(Lambda) << '\n';
    std::cout << "Size of normal lambda struct: " << sizeof(NormalLambdaStruct) << '\n';
    std::cout << "Size with no unique address lambda: " << sizeof(NoUniqueAddressLambda) << '\n';
    return 0;
}