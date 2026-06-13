#include <cstdio>
#include <iostream>
#include <type_traits>

// An example of curiously recurring template pattern (CTRP)
// Its useful for "inheritance" at compile time

template <typename T>
class Singleton {
   public:
    static T& getInstance() {
        // Use static instance inside method, lazily evaluated so avoids static
        // initialization fiasco issues
        static T instance;
        return instance;
    }

   protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

class A : public Singleton<A> {
   public:
    double d{};

   private:
    friend Singleton;
    A() = default;
};

template <typename T>
class Writer {
   public:
    template <size_t N>
    void write(const char (&arr)[N]) {
        static_cast<T*>(this)->writeImpl(arr);
    }
};

class CoutWriter : public Writer<CoutWriter> {
   public:
    template <size_t N>
    void writeImpl(const char (&arr)[N]) {
        std::cout << arr << '\n';
    }
};

class PrintfWriter : public Writer<PrintfWriter> {
   public:
    template <size_t N>
    void writeImpl(const char (&arr)[N]) {
        printf("%s\n", arr);
    }
};

template <typename T>
void printStuff(Writer<T>& w) {
    w.write("Stuff");
}

int main() {
    {
        A& a = A::getInstance();
        std::cout << a.d << '\n';
        a.d++;
    }

    A::getInstance().d++;

    A& a = A::getInstance();
    std::cout << a.d << '\n';  // If its a singleton should be incremented twice

    // A a2{};   // Fails
    // A a3{a};  // Fails since base copy constructor is deleted

    CoutWriter coutWriter{};
    PrintfWriter printfWriter{};

    // This looks like polymorphism but everything gets resolved at compile time.
    // Its still not quite as flexible as polymorphism its not like you can have a vector of base
    // class pointers and resolve it at compile time obv. But you've successfully got inheritance
    // hierarchies and code reuse which is pretty cool.
    printStuff(coutWriter);
    printStuff(printfWriter);
}