#include <cassert>
#include <charconv>
#include <iostream>
#include <meta>
#include <ranges>
#include <vector>

constexpr char raw_data[] = {
#embed "example.csv"
    , 0  // Null terminate
};

template <size_t N>
struct CompString {
    constexpr CompString(const char (&arr)[N]) { std::copy_n(arr, N, data.data()); }
    consteval auto string_view() const { return std::string_view(data.data(), N - 1); }
    std::array<char, N> data;
};

constexpr std::string_view csv{raw_data};

struct Row;  // incomplete initially, we parse the csv then complete it after

consteval std::vector<std::string_view> split(std::string_view s, char delim) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == delim) {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}

consteval std::string_view sanitize(std::string_view s) {
    std::string_view filtered{s};

    auto a = s.begin();
    auto b = s.end();

    size_t prefix_whitespace{};
    while (a != b) {
        if (*a == ' ') {
            ++prefix_whitespace;
            ++a;
        } else
            break;
    }

    size_t postfix_whitespace{};
    while (a != b) {
        if (*b == ' ') {
            ++postfix_whitespace;
            --b;
        } else
            break;
    }

    filtered.remove_prefix(prefix_whitespace);
    filtered.remove_suffix(postfix_whitespace);
    return filtered;
}

consteval std::meta::info view_to_type(std::string_view t) {
    if (t == "size_t") return ^^size_t;
    if (t == "int") return ^^int;
    if (t == "double") return ^^double;
    if (t == "float") return ^^float;
    if (t == "std::string_view") return ^^std::string_view;
    throw "Unkown type, add it to mapping";
}

consteval size_t parse_size_t(std::string_view s) {
    size_t result{};
    for (char c : s) {
        result = result * 10 + (c - '0');
    }
    return result;
}

template <std::meta::info Member>
consteval auto parse_field(std::string_view s) {
    constexpr auto T = std::meta::dealias(std::meta::type_of(Member));
    // Very fucked. view_to_type matches size_t with a ^^size_t.
    // But a size_t is just an implementation defined typedef unsigned integer,
    // large enough to hold the maximum size of any object in memory.
    // On my machine this aliases an unsigned long, so the compiler resolves the alias.
    // This means it returns a ^^unsigned long.
    // Now if we compare our type which is an ^^unsigned_long with ^^size_t,
    // despite size_t being an alias for it its not equal so we get a bug.
    // So we need to dealias our size_t before the comparison.
    if constexpr (T == std::meta::dealias(^^size_t)) return parse_size_t(s);
    throw "Unkown type, add it to field parsing";
}

constexpr size_t NUM_ROWS = []() -> size_t {
    size_t count{};
    auto a = csv.begin();
    auto b = csv.end();
    for (; a != b; ++a) {
        if (*a == '\n') ++count;
    }
    return count;
}();

consteval {
    auto lines = split(csv, '\n');
    auto headers = split(lines[0], ',');

    std::vector<std::meta::info> specs;
    for (size_t i = 0; i < headers.size(); ++i) {
        auto pair = split(headers[i], '.');
        if (pair.size() != 2) {
            throw "Expected name.type format";
        }
        std::meta::info type = view_to_type(sanitize(pair[1]));
        specs.push_back(data_member_spec(type, {.name = sanitize(pair[0])}));
    }
    define_aggregate(^^Row, specs);
}

constexpr std::array<Row, NUM_ROWS> data = [] {
    std::array<Row, NUM_ROWS> data;

    auto lines = split(csv, '\n');
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
    return 0;
}