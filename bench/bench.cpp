#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <random>
#include <string>
#include <vector>

#include "rolldown/rolldown.hpp"
#include "spec_helpers.hpp"
#include "xoshiro/xoshiro.hpp"

// Each BENCHMARK returns its result so the optimizer can't delete the work being timed.

TEST_CASE("cost of each step", "[benchmark]")
{
    const auto table = rolldown::buildDropTable(makeSpec({{3.5, {60, 40}}, {20, {50, 50}}, {76.5, {100}}})).table;
    std::mt19937_64 mt{12345};
    XoshiroCpp::Xoshiro256PlusPlus xo{12345};

    BENCHMARK("rng only: mt19937_64") { return mt(); };
    BENCHMARK("rng only: xoshiro256++") { return xo(); };
    BENCHMARK("rollIndex: mt19937_64") { return rolldown::rollIndex(table, mt); };
    BENCHMARK("rollIndex: xoshiro256++") { return rolldown::rollIndex(table, xo); };
}

TEST_CASE("rollIndex vs table size", "[benchmark]")
{
    XoshiroCpp::Xoshiro256PlusPlus xo{12345};
    for (int n : {2, 8, 32, 128, 1000})
    {
        const auto table = rolldown::buildDropTable(makeEvenSpec(n)).table;
        BENCHMARK("rollIndex, " + std::to_string(n) + " packs") { return rolldown::rollIndex(table, xo); };
    }
}

TEST_CASE("roll vs rollIndex", "[benchmark]")
{
    XoshiroCpp::Xoshiro256PlusPlus xo{12345};
    const auto table = rolldown::buildDropTable(makeEvenSpec(100)).table;
    BENCHMARK("roll, 100 packs") { return &rolldown::roll(table, xo); };
    BENCHMARK("rollIndex, 100 packs") { return rolldown::rollIndex(table, xo); };
}

TEST_CASE("rollIndexes with iterator provided vs vector returned")
{
    XoshiroCpp::Xoshiro256PlusPlus xo{12345};
    const auto table = rolldown::buildDropTable(makeEvenSpec(100)).table;

    size_t loopOut[1000];
    BENCHMARK("rollIndex in a loop, 100 packs, 1000 rolls")
    {
        for (size_t i = 0; i < 1000; ++i)
            loopOut[i] = rolldown::rollIndex(table, xo);
        return loopOut[999];
    };

    BENCHMARK("rollIndexes with returned vector, 100 packs, 1000 rolls") { return rolldown::rollIndexes(table, xo, 1000); };

    size_t cStyle[1000];
    BENCHMARK("rollIndexes with provided c-style array, 100 packs, 1000 rolls") { return rolldown::rollIndexes(table, xo, 1000, cStyle); };

    std::vector<size_t> v;
    v.reserve(1000);
    BENCHMARK("rollIndexes with provided vector, 100 packs, 1000 rolls")
    {
        v.clear(); // capacity stays at 1000, so this measures reuse, not growth
        rolldown::rollIndexes(table, xo, 1000, std::back_inserter(v));
        return v.back();
    };
}
