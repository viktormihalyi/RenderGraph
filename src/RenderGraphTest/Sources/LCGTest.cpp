#include "RenderGraph/Utils/Assert.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <random>


using Empty = ::testing::Test;


uint64_t Forrest_G (const uint64_t k, const uint64_t seed, const uint64_t g, const uint64_t m)
{
    uint64_t G = 1;
    uint64_t h = g;

    uint64_t i = (k + m) % m;

    while (i > 0) {
        if (i % 2 == 1) {
            G = (G * h) % m;
        }
        h = (h * h) % m;
        i = i / 2;
    }

    return (seed * G) % m;
}


uint64_t Forrest_C (const uint64_t k, const uint64_t seed, const uint64_t g, const uint64_t c, const uint64_t m)
{
    uint64_t C = seed % m;
    uint64_t f = c;
    uint64_t h = g;
    uint64_t i = (k + m) % m;

    while (i > 0) {
        if (i % 2 == 1) {
            C = (C * h + f) % m;
        }
        f = (f * (h + 1)) % m;
        h = (h * h) % m;
        i = i / 2;
    }

    return C;
}


TEST_F (Empty, ArbitraryStrideLCG)
{
    double   sum     = 0.0;
    int      count   = 0;
    uint64_t modolus = 2147483647; //(static_cast<uint64_t> (1) << 31) - 1; // 2^31 - 1
    uint64_t mul_a   = 48271;      // minstd_rand
    // uint64_t inc_c   = 0;

    for (int i = 10; i < 100000; ++i) {
        double val  = static_cast<double> (Forrest_G (i, 634, mul_a, modolus));
        // double val2 = static_cast<double> (Forrest_C (i, 634, mul_a, 0, modolus));
        const double perc = val / modolus;
        sum += perc;
        ++count;
    }

    std::cout << sum / count << std::endl;
}


TEST_F (Empty, ArbitraryStrideLCG_TESTWITHACTUAl)
{
    constexpr uint64_t modolus = 2147483648; //(static_cast<uint64_t> (1) << 31) - 1; // 2^31 - 1
    constexpr uint64_t mul_a   = 48271;      // minstd_rand
    constexpr uint64_t inc_c   = 0;
    constexpr uint64_t seed   = 634;
    
    std::linear_congruential_engine<uint32_t, mul_a, inc_c, modolus> e1 (seed);
    std::uniform_int_distribution<uint32_t> uniform_dist (0, std::numeric_limits<uint32_t>::max ());
    
    std::vector<size_t> fromstd;
    std::vector<size_t> fromfor;

    for (size_t i = 1; i < 24; ++i) {
        fromfor.push_back (Forrest_G (i, seed, mul_a, modolus));
        fromstd.push_back (uniform_dist (e1));
    }
}