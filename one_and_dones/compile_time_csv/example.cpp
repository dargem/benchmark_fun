#include <cassert>
#include <charconv>
#include <iostream>
#include <meta>
#include <ranges>
#include <vector>

constexpr char raw_data[] = {
#embed "example.csv"
    , 0  // Null terminates the file
};

constexpr std::string_view csv{raw_data};  // Can get a string_view to it for easier to use

struct Row;  // We declare there's a type Row but its incomplete

// Using define_aggregate we will define it using the csv's data

// We want a handy way to split the data, our raw_data is an array, endlines are \n
// Then delimiters between entries on the same line is ,
// Then delimiters between the name and value is a .

consteval std::vector<std::string_view> split(std::string_view line, char delimiter) {
    std::vector<std::string_view> out;
    size_t start{};

    for (size_t i{}; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == delimiter) {
            out.push_back(line.substr(start, i - start));
            start = i + 1;
        }
    }

    return out;
}

// And we want a way to remove leading/trailing whitespace since we allow that
consteval std::string_view sanitize(std::string_view line, char skip = ' ') {
    size_t begin{};
    size_t end{line.size()};

    for (; begin < end && line[begin] == skip; ++begin);
    for (; begin < end && line[end - 1] == skip; --end);

    return line.substr(begin, end - begin);
}

// Now while we can parse our line and get out a "name" "type" pair strings,
// we need to map our string which is the name of a type into that type.
// There's no way except making our own mapping I think sadly.

// ^^ is the syntax for the static reflection operator, it gets a std::meta::info from the given
// type
consteval std::meta::info string_to_type(std::string_view name) {
    if (name == "size_t") return ^^size_t;
    if (name == "int") return ^^int;
    if (name == "double") return ^^double;
    if (name == "float") return ^^float;
    if (name == "std::string_view") return ^^std::string_view;
    throw "Unknown type, add it to mapping";
}

// Now we can parse our fields, we're going to use a consteval block here
// It basically guarantees everything done in this block is done at compile time

consteval {
    // The consteval block forcing this to be done at compile time is needed
    // For example split returns a std::vector at compile time
    // This is fine if its all done in a constant evaluated context
    // But it can't "escape out" of that

    // We remove trailing \n in case the file has empty lines
    auto lines = split(sanitize(csv, '\n'), '\n');
    auto headers = split(lines[0], ',');

    std::vector<std::meta::info> specs;
    for (size_t i{}; i < headers.size(); ++i) {
        auto pair = split(headers[i], '.');

        if (pair.size() != 2) {
            throw "Expects name.type format";
            // Could do a lot more sanitization and checking but this is more of an example prob
        }

        std::meta::info type = string_to_type(sanitize(pair[1]));

        // Our type is the member's type... The name will be the member's identifier
        specs.push_back(data_member_spec(type, {.name = sanitize(pair[0])}));
    }

    // And that's step 3 also, defining Row with our identifiers and associated types is easy
    define_aggregate(^^Row, specs);
}

// Could modify this pretty easily to parse signed / unsigned ints of any size
consteval size_t parse_size_t(std::string_view s) {
    s = sanitize(s);
    size_t result{};
    for (char c : s) {
        result = result * 10 + (c - '0');
    }
    return result;
}

template <std::meta::info Member>
consteval auto parse_field(std::string_view field) {
    // We just make a mapping, parsing our data into a given type

    // Get dealiased type of member
    constexpr auto T = std::meta::dealias(std::meta::type_of(Member));

    // We use deal
    if (T == std::meta::dealias(^^size_t)) return parse_size_t(sanitize(field, ' '));
    // ... support for other types delegating to its parsing function

    throw "Unkown type, add it to field parsing";
}

// We're just going to use immediately evaluated lambdas
// I think its the cleanest way to initialize a constexpr variable
constexpr size_t NUM_ROWS = []() { return split(sanitize(csv, '\n'), '\n').size() - 1; }();

constexpr std::array<Row, NUM_ROWS> data = [] {
    std::array<Row, NUM_ROWS> data;

    auto lines = split(sanitize(csv, '\n'), '\n');
    constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^Row, std::meta::access_context::unchecked()));

    for (size_t i{1}; i < lines.size(); ++i) {
        auto elements = split(lines[i], ',');

        size_t j{};
        template for (constexpr auto member : members) {
            data[i - 1].[:member:] = parse_field<member>(sanitize(elements[j]));
            ++j;
        }
    }

    return data;
}();

template <size_t N>
struct CompString {
    constexpr CompString(const char (&arr)[N]) { std::copy_n(arr, N, data.data()); }
    consteval auto string_view() const { return std::string_view(data.data(), N - 1); }
    std::array<char, N> data;
};

template <CompString s>
consteval auto get(const Row& r) {
    constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^Row, std::meta::access_context::unchecked()));
    template for (constexpr auto member : members) {
        if (std::meta::identifier_of(member) == s.string_view()) {
            return r.[:member:];
        }
    }
    throw "no member with that name";
}

int main() {
    std::cout << "Row Names & Types" << '\n';
    constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^Row, std::meta::access_context::unchecked()));
    template for (constexpr auto member : members) {
        std::cout << std::meta::identifier_of(member) << ": "
                  << std::meta::display_string_of(std::meta::type_of(member)) << '\n';
    }

    std::cout << "Data" << '\n';

    for (const Row& row : data) {
        template for (constexpr auto member : members) {
            std::cout << std::meta::identifier_of(member) << ": " << row.[:member:] << ' ';
        }
        std::cout << '\n';
    }

    std::cout << get<"AGE">(data[2]) << '\n';
    std::cout << data[2].AGE << '\n';
    return 0;
}