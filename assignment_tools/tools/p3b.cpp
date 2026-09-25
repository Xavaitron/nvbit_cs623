#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

constexpr unsigned NUM_SETS = 32;
constexpr unsigned NUM_WAYS = 32;

struct CacheSet {
    // Valid entries occupy [0, used).
    // Index 0 is most recently used; used-1 is least recently used.
    std::array<uint64_t, NUM_WAYS> tags{};
    unsigned used = 0;
};

struct Cache {
    std::array<CacheSet, NUM_SETS> sets;
};

struct Model {
    unsigned sm_count;
    std::vector<Cache> caches;

    uint64_t hits = 0;
    uint64_t misses = 0;

    explicit Model(unsigned n) : sm_count(n), caches(n) {}

    void access(uint64_t cta, uint64_t address) {
        const unsigned sm = cta % sm_count;
        const uint64_t block = address >> 7;
        const unsigned set_index = block & (NUM_SETS - 1);
        const uint64_t tag = block >> 5;

        CacheSet &set = caches[sm].sets[set_index];

        unsigned position = 0;
        while (position < set.used && set.tags[position] != tag) {
            ++position;
        }

        if (position < set.used) {
            ++hits;
        } else {
            ++misses;

            if (set.used < NUM_WAYS) {
                // Insert into an available way.
                position = set.used;
                ++set.used;
            } else {
                // Replace the least recently used way.
                position = NUM_WAYS - 1;
            }
        }

        // Move the accessed/inserted tag to the MRU position.
        for (unsigned i = position; i > 0; --i) {
            set.tags[i] = set.tags[i - 1];
        }
        set.tags[0] = tag;
    }
};

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

    // Independent, initially empty caches for each configuration.
    std::vector<Model> models;
    models.reserve(7);
    for (unsigned n = 1; n <= 64; n *= 2) {
        models.emplace_back(n);
    }

    uint64_t references = 0;
    uint64_t accesses = 0;
    uint64_t line_number = 0;
    std::string line;

    while (std::getline(input, line)) {
        ++line_number;

        if (line.compare(0, 4, "MEM ") != 0) continue;

        std::istringstream row(line);
        std::string marker;
        uint64_t cta;
        unsigned count;

        if (!(row >> marker >> cta >> count) ||
            count == 0 || count > 32) {
            std::cerr << "Invalid record at line "
                      << line_number << '\n';
            return 1;
        }

        uint64_t addresses[32];

        for (unsigned i = 0; i < count; ++i) {
            if (!(row >> std::hex >> addresses[i])) {
                std::cerr << "Invalid address at line "
                          << line_number << '\n';
                return 1;
            }
        }

        std::string extra;
        if (row >> extra) {
            std::cerr << "Extra fields at line "
                      << line_number << '\n';
            return 1;
        }

        // Preserve trace order. Each thread address is one cache access.
        for (unsigned i = 0; i < count; ++i) {
            for (auto &model : models) {
                model.access(cta, addresses[i]);
            }
        }

        ++references;
        accesses += count;
    }

    if (input.bad() || references == 0) {
        std::cerr << "Trace read failed or no memory records found\n";
        return 1;
    }

    std::cerr << "Warp memory references: " << references << '\n'
              << "Thread memory accesses: " << accesses << '\n';

    std::cout << "SMs,Hits,Misses,TotalAccesses\n";

    for (const auto &model : models) {
        if (model.hits + model.misses != accesses) {
            std::cerr << "Internal accounting error\n";
            return 1;
        }

        std::cout << model.sm_count << ','
                  << model.hits << ','
                  << model.misses << ','
                  << accesses << '\n';
    }

    return 0;
}
