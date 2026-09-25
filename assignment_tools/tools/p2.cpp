#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <pc_trace.log>\n";
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open " << argv[1] << '\n';
        return 1;
    }

    std::unordered_set<uint64_t> instructions;
    std::unordered_set<uint64_t> blocks;
    uint64_t records = 0;
    uint64_t line_number = 0;
    std::string line;

    while (std::getline(input, line)) {
        ++line_number;

        // Ignore application output and NVBit diagnostics.
        if (line.compare(0, 3, "PC ") != 0) continue;

        const char *start = line.c_str() + 3;
        char *end = nullptr;
        errno = 0;

        const unsigned long long value =
            std::strtoull(start, &end, 16);

        const bool range_error = errno == ERANGE;
        const bool no_digits = end == start;

        while (*end &&
               std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }

        if (line.compare(0, 5, "PC 0x") != 0 ||
            range_error || no_digits || *end != '\0') {
            std::cerr << "Invalid PC record at line "
                      << line_number << '\n';
            return 1;
        }

        const uint64_t pc = static_cast<uint64_t>(value);

        instructions.insert(pc);
        blocks.insert(pc >> 5);  // Divide by 32.
        ++records;
    }

    if (input.bad() || records == 0) {
        std::cerr << "Trace read failed or no PC records found\n";
        return 1;
    }

    std::cout
        << "Warp instructions: " << records << '\n'
        << "Unique instructions: " << instructions.size() << '\n'
        << "Unique 32-byte instruction cache blocks: "
        << blocks.size() << '\n';

    return 0;
}
