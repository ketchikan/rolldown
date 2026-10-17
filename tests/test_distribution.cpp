#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <random>
#include <vector>

#include "rolldown/rolldown.hpp"
#include "spec_helpers.hpp"
#include "xoshiro/xoshiro.hpp"

// Roll `n` times and count how often each pack index comes up.
template <class Rng>
std::vector<long> countRolls(const rolldown::DropTable &table, Rng &rng, long n)
{
    std::vector<long> counts(table.size(), 0);
    for (long i = 0; i < n; ++i)
        ++counts[rolldown::rollIndex(table, rng)];
    return counts;
}

// Every observed count must be within 5 standard deviations of what the spec says.
// With a fixed seed the result is the same every run, so this never flakes.
void checkDistribution(const std::vector<long> &counts, const std::vector<double> &expected, long n)
{
    REQUIRE(counts.size() == expected.size());
    for (size_t i = 0; i < counts.size(); ++i)
    {
        const double p = expected[i];
        const double mean = static_cast<double>(n) * p;
        const double sd = std::sqrt(static_cast<double>(n) * p * (1.0 - p));

        INFO("pack " << i << ": expected ~" << mean << " (p = " << p << "), got " << counts[i]);
        CHECK(std::abs(static_cast<double>(counts[i]) - mean) <= 5.0 * sd); // sd = 0 for 0%/100%: must be exact
    }
}

TEMPLATE_TEST_CASE("rolls follow the configured probabilities", "[distribution]",
                   std::mt19937_64, XoshiroCpp::Xoshiro256PlusPlus)
{
    constexpr long N = 10'000'000;
    TestType rng{12345};

    SECTION("mixed tiers (items3)")
    {
        const auto spec = makeSpec({{3.5, {60, 40}}, {20, {50, 50}}, {76.5, {100}}});
        const auto result = rolldown::buildDropTable(spec);
        REQUIRE(result);
        checkDistribution(countRolls(result.table, rng, N), expectedProbabilities(spec), N);
    }
    SECTION("a 0% pack never drops")
    {
        const auto spec = makeSpec({{100, {0, 100}}});
        const auto result = rolldown::buildDropTable(spec);
        REQUIRE(result);
        checkDistribution(countRolls(result.table, rng, N), expectedProbabilities(spec), N);
    }
    SECTION("a very rare pack (0.01% of 1%)")
    {
        const auto spec = makeSpec({{1, {0.01, 99.99}}, {99, {100}}});
        const auto result = rolldown::buildDropTable(spec);
        REQUIRE(result);
        checkDistribution(countRolls(result.table, rng, N), expectedProbabilities(spec), N);
    }
    SECTION("a large pack (100 items)")
    {
        const auto spec = makeEvenSpec(100);
        const auto result = rolldown::buildDropTable(spec);
        REQUIRE(result);
        checkDistribution(countRolls(result.table, rng, N), expectedProbabilities(spec), N);
    }
}
