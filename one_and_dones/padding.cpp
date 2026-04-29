#include <iostream>

// int is 4 bytes, double 8 bytes on my machine. char being 1 byte is part of the standard though. A
// double doesn't technically have to be 8 bytes but it basically stands for double precision
// IEEE-754 format so its basically always going to be 8 bytes.

struct IntIntDouble {
    int int_val_1;      // needs to be 4 byte aligned [0:3]
    int int_val_2;      // needs to be 4 byte aligned [4:7]
    double double_val;  // needs to be 8 byte aligned [8:15]
};  // struct has the alignment of its largest elements alignment which is 8 bytes here. So [0:15]
    // works out with no end padding as thats a multiple of 8.

struct IntDoubleInt {
    int int_val_1;      // needs to be 4 byte aligned [0:3]
    double double_val;  // needs to be 8 byte aligned [8:15]
    int int_val_2;      // needs to be 4 byte aligned [16:19]
};  // struct has 8 byte alignment, but it ends at 19 (20 bytes used), this isn't a multiple of 20
    // so we need to add some padding at the end. So it would take [0:23] or 24 bytes total

// on all modern systems doubles are 8 byte aligned, on some older 32 bit linux systems though
// doubles actually used to be 4 byte aligned to avoid potential wasted space like seen here

static_assert(sizeof(IntIntDouble) == 16);
static_assert(sizeof(IntIntDouble) == (sizeof(int) + sizeof(int) + sizeof(double)));

static_assert(sizeof(IntDoubleInt) != 16);
static_assert(sizeof(IntDoubleInt) != (sizeof(int) + sizeof(int) + sizeof(double)));

struct CharInt {
    char char_val;  // takes [0] as 1 bytes
    int int_val;    // [4:7] as 4 bytes
};

static_assert(sizeof(CharInt) == 8);

struct IntChar {
    int int_val;    // takes [0:3] as 4 bytes
    char char_val;  // [4] as 1 bytes so 5 bytes used
};  // this struct has a 4 byte alignment though as it has the same alignment of the strictest
    // aligned member. So while this is 5 bytes, there's going to be 3 bytes of padding at the end
    // to make this 8 bytes. This is 3/8ths of the objects space going to padding

static_assert(sizeof(IntChar) == 8);

// so alternatively lets replace the integer with 4 bytes
struct FiveChar {
    char int_val_0;
    char int_val_1;
    char int_val_2;
    char int_val_3;
    char int_val_4;  // 5 bytes total used
};  // this struct has a 1 byte alignment though as it has the same alignment of the strictest. So
    // there's no padding needed at the end and its just going to be 5 bytes

static_assert(sizeof(FiveChar));

int main() {
    std::cout << "Size of IntIntDouble is: " << sizeof(IntIntDouble) << std::endl;  // 16
    std::cout << "Size of IntDoubleInt is: " << sizeof(IntDoubleInt) << std::endl;  // 24
    return 0;
}

// We have effectively saved 8 bytes from good layout choice