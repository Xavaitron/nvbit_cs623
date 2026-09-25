#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <memory_trace.log>\n";
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open " << argv[1] << '\n';
        return 1;
    }

    uint64_t references = 0;
    uint64_t accesses = 0;
    uint64_t line_number = 0;
    long double divergence_sum = 0;
    std::string line;

    while (std::getline(input, line)) {
        ++line_number;

        // Skip NVBit diagnostics and application output.
        if (line.compare(0, 4, "MEM ") != 0) continue;

        std::istringstream row(line);
        std::string marker;
        uint64_t cta;
        unsigned count;

        if (!(row >> marker >> cta >> count) ||
            count == 0 || count > 32) {
            std::cerr << "Invalid record at line " << line_number << '\n';
            return 1;
        }

        uint64_t blocks[32];

        for (unsigned i = 0; i < count; ++i) {
            uint64_t address;

            if (!(row >> std::hex >> address)) {
                std::cerr << "Invalid address at line "
                          << line_number << '\n';
                return 1;
            }

            // Each access stays within its starting 128-byte block.
            blocks[i] = address >> 7;
        }

        std::string extra;
        if (row >> extra) {
            std::cerr << "Extra fields at line " << line_number << '\n';
            return 1;
        }

        std::sort(blocks, blocks + count);

        const unsigned unique_blocks =
            std::unique(blocks, blocks + count) - blocks;

        // Average the ratios of individual warp references.
        divergence_sum +=
            static_cast<long double>(unique_blocks) / count;

        ++references;
        accesses += count;
    }

    if (input.bad() || references == 0) {
        std::cerr << "Trace read failed or no memory records found\n";
        return 1;
    }

    std::cout
        << "Warp memory references: " << references << '\n'
        << "Thread memory accesses: " << accesses << '\n'
        << std::fixed << std::setprecision(12)
        << "Memory divergence: "
        << divergence_sum / references << '\n';

    return 0;
}
