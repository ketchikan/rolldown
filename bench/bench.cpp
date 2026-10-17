#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <random>
#include <string>

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
