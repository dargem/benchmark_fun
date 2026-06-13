#include <iostream>

template <typename T>
struct Foo {
    void print() { std::cout << "Hello World\n"; }
};

// We have omitted a definition
struct Undefined;

int main() {
    Foo<Undefined> a{};
    a.print();
}