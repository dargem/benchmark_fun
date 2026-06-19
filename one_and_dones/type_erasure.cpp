/*
The drawback to stuff like CRTP and template polymorphism is stuff like being unable to place
disparate types into a collection. Through using interfaces this is possible, but its not unless you
know the types of each index at compile time like for a a std::tuple but we want containers but we
want containers that can accept types only known at runtime.
*/

#include <concepts>
#include <iostream>
#include <memory>
#include <vector>

class SoundMaker {
   public:
    virtual const char* make_sound() const = 0;
};

// Then say we have some sound makers like Car, Dog and Speakers

class Car {
   public:
    const char* make_sound() const { return "Vroom"; }
};

class CarWrapper : public SoundMaker {
   private:
    Car car;

   public:
    const char* make_noise() const { return car.make_sound(); }
};

// Now we can work with Car but this sure is a lot of boiler plate. But through templates we can
// just have the compiler generate the boilerplate for us.

template <typename T>
class SoundMakerWrapper : SoundMaker {
   private:
    const T* t;

   public:
    const char* make_sound() const { return t->make_sound(); }
};

// Now we can have SoundMakerWrapper template our class and they become polymorphic to SoundMaker.
// But its preferable that this type erasure is all abstracted away so lets do that

// We just do this here to avoid some naming collisions with above
namespace separator {

template <typename T>
concept CanMakeSound = requires(T t) {
    { t.make_sound() } -> std::same_as<const char*>;
};

class SoundMakers {
   private:
    class SoundMaker {
       public:
        virtual const char* make_sound() const = 0;
        virtual ~SoundMaker() = default;
    };

    template <typename T>
        requires CanMakeSound<T>
    class SoundMakerWrapper : public SoundMaker {
       private:
        T* t;

       public:
        SoundMakerWrapper() = default;
        SoundMakerWrapper(T* t) : t(t) {}

        const char* make_sound() const override { return t->make_sound(); }
    };

   public:
    template <typename T>
        requires CanMakeSound<T>
    void add_sound_maker(T* t) {
        sound_makers.push_back(std::make_unique<SoundMakerWrapper<T>>(t));
    }

    void make_noises() {
        for (const auto& maker : sound_makers) {
            std::cout << maker->make_sound() << '\n';
        }
    }

   private:
    std::vector<std::unique_ptr<SoundMaker>> sound_makers;
};

}  // namespace separator

class Dog {
   public:
    const char* make_sound() const { return "Woof"; }
};

int main() {
    separator::SoundMakers sound_maker{};
    Dog d{};
    Car c{};
    sound_maker.add_sound_maker(&d);
    sound_maker.add_sound_maker(&c);

    sound_maker.make_noises();
}