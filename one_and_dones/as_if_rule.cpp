#include <iostream>

class A {
   public:
    A() {
        static size_t constructions{};
        std::cout << "Constructed N: " << ++constructions << '\n';
    }

    ~A() {
        static size_t deletions{};
        std::cout << "Deleted N: " << ++deletions << '\n';
    }
};

// A factory for the class A
class AFactory {
   public:
    A create() {
        // We are returning a prvalue (pure right value). If a caller wants to do something like etc
        // `A a = factory.create()`, this naively could do an extra operation. First create()
        // creates an A object, then copy construction is used for our caller. Note that while the
        // assignment operator is used here it still calls copy construction, assignment is only
        // used if there is already an instance of the object. This copying is extra work, while
        // this isn't expensive here it would be preferable for factory.create() to just create the
        // object at where the caller expects to receive it. This is done through RVO (return value
        // optimization) which is guaranteed starting with c++ 17 under certain conditions. This is
        // guaranteed, so even if construction/destruction has observable behaviour the copy must be
        // elided.
        return A{};
        // If we return a prvalue (pure right value) there is guaranteed RVO. This must be a pure
        // right value not just an ordinary right value. This makes sense because something like
        // std::move is basically an rvalue cast (more like xvalue) on an lvalue object. This is
        // clearly quite a different case.
    }
};

class AFactory2 {
   public:
    A create() {
        A a{};
        // This is a lvalue not a prvalue so RVO doesn't apply. For lvalues compilers can do NRVO
        // (named return value optimization). This similarly breaks the as-if rule and the compiler
        // can (but doesn't need to) elide the copy. This means different optimization levels can
        // result in different behaviour if construction/destruction has observable side effects.
        return a;
    }
};

int main() {
    AFactory factory{};
    AFactory2 factory2{};

    {
        A a = factory.create();
        // In this case NRVO is applying at least for my version of GCC
        A a2 = factory2.create();
    }
}