#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <string_view>
#include <algorithm>
#include <locale>
#include <array>
#include <exception>


using Table = std::pair<std::vector<std::string>, std::vector<std::vector<double>>>;


// The hack https://stackoverflow.com/a/7304184
class ColonAsSep final : public std::ctype<char> {
public:
    ColonAsSep() : std::ctype<char>(ColonAsSep::get_table()) {
    }

    // USE: cin.imbue(locale(cin.getloc(), &colon_as_sep));
    static mask const* get_table() {
        static std::array<std::ctype_base::mask, 1 << (8 * sizeof(char))> rc;
        rc[' '] = std::ctype_base::space;
        rc[','] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        return rc.data();
    }
};

template<typename T>
std::vector<T> split_parse_raw(const std::string&str) {
    std::istringstream sin(str);
    sin.imbue(std::locale(sin.getloc(), new ColonAsSep)); // doesnt leak, (c) Valgrind
    std::vector<T> ret;
    T val;
    size_t column_i = 1;
    try {
        while (sin >> val) {
            ret.push_back(val);
            column_i += 1;
        }
        if (!sin.eof()) {
            throw std::runtime_error("parsing failed");
        }
    }
    catch (std::exception&err) {
        throw std::runtime_error(std::string("column ") + std::to_string(column_i) + ": " + err.what());
    }
    return ret;
}


Table read_table(const std::string&filename) {
    std::vector<std::string> head;
    std::vector<std::vector<double>> data;

    std::ifstream fin(filename);
    if (std::string line; getline(fin, line)) {
        head = split_parse_raw<std::string>(line);
    }
    if (head.empty()) {
        throw std::runtime_error("table has empty head");
    }

    std::string line;
    size_t line_i = 0;
    while (std::getline(fin, line)) {
        line_i += 1;
        try {
            auto parsed = split_parse_raw<double>(line);
            if (parsed.size() != head.size()) {
                throw std::runtime_error("line column size != head size");
            }
            data.push_back(std::move(parsed));
        }
        catch (std::runtime_error&err) {
            // std::format C++23 would be lovely in here
            throw std::runtime_error(std::string("error parsing line ") + std::to_string(line_i) + ": " + err.what());
        }
    }

    return {std::move(head), std::move(data)}; // RAII?
}

std::string left_pad(const std::string_view what, const size_t count, const char filler) {
    if (what.size() >= count) {
        return std::string{what};
    }
    return std::string(count - what.size(), filler) + std::string(what);
}

void print_table(const Table&table, const size_t min_padding = 12, std::ostream&out = std::cout) {
    const auto&[head, data] = table;
    for (const auto&name: head) {
        out << left_pad(name, min_padding, ' ') << ' ';
    }
    for (const auto&line: data) {
        out << '\n';
        for (size_t i = 0; i < line.size(); i++) {
            out << left_pad(std::to_string(line[i]), std::max(head[i].size(), min_padding), ' ') << ' ';
        }
    }
}

int main(const int argc, char* argv[]) {
    std::string filename = "input.csv";
    if (argc > 1) {
        filename = argv[1];
    }

    const auto table = read_table(filename);
    print_table(table);
}
