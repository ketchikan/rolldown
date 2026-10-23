#include <catch2/catch_test_macros.hpp>
#include <random>

#include "rolldown/rolldown.hpp"
#include "spec_helpers.hpp"
#include "json_loader.hpp"

// RUN: ctest --test-dir build --output-on-failure

using rolldown::buildDropTable;
using rolldown::BuildError;
using rolldown_json::LoadError;
using rolldown_json::loadTablesFromJson;
using rolldown_json::parseTables;

TEST_CASE("a valid spec builds", "[build]")
{
    const auto result = buildDropTable(makeSpec({{3.5, {60, 40}}, {20, {50, 50}}, {76.5, {100}}}));

    REQUIRE(result);
    CHECK(result.table.size() == 5);
}

TEST_CASE("invalid specs report the right error and tier", "[build]")
{
    SECTION("no tiers")
    {
        CHECK(buildDropTable(rolldown::DropTableSpec{}).error == BuildError::NoTiers);
    }
    SECTION("a tier with no packs")
    {
        const auto result = buildDropTable(makeSpec({{50, {100}}, {50, {}}}));
        CHECK(result.error == BuildError::EmptyTier);
        CHECK(result.tierIndex == 1);
    }

    SECTION("a negative pack probability")
    {
        const auto result = buildDropTable(makeSpec({{100, {-10, 110}}}));
        CHECK(result.error == BuildError::OutOfRange);
    }
    SECTION("a negative tier probability")
    {
        const auto result = buildDropTable(makeSpec({{-100, {110, 10}}}));
        CHECK(result.error == BuildError::OutOfRange);
    }
    SECTION("tiers that don't add up to 100%")
    {
        CHECK(buildDropTable(makeSpec({{50, {100}}, {40, {100}}})).error == BuildError::TierSumNot100);
    }
    SECTION("tiers whose item errors cancel out (regression)")
    {
        const auto result = buildDropTable(makeSpec({{50, {60, 60}}, {50, {40, 40}}}));
        CHECK(result.error == BuildError::ItemSumNot100);
        CHECK(result.tierIndex == 0);
    }
    SECTION("item pack probability defined as NaN")
    {
        const auto result = buildDropTable(makeSpec({{100, {NAN, 100}}}));
        CHECK(result.error == BuildError::OutOfRange);
    }
    SECTION("item tier probability defined as NaN")
    {
        const auto result = buildDropTable(makeSpec({{100, {100}}, {NAN, {100}}}));
        CHECK(result.error == BuildError::OutOfRange);
    }
    SECTION("item pack probability has too much precision (3+ decimal places)")
    {
        const auto result = buildDropTable(makeSpec({{50, {99.999, 0.001}}, {50, {0.001, 99.999}}}));
        CHECK(result.error == BuildError::TooPrecise);
    }
    SECTION("item tier probability has too much precision (3+ decimal places)")
    {
        const auto result = buildDropTable(makeSpec({{99.999, {100}}, {0.001, {100}}}));
        CHECK(result.error == BuildError::TooPrecise);
    }
}

TEST_CASE("a batch gives the same rolls as single calls", "[roll]")
{
    const auto built = buildDropTable(makeSpec({{100, {25, 75}}}));
    REQUIRE(built);
    std::mt19937_64 a{42}, b{42}, c{42};

    std::vector<size_t> single;
    for (int i = 0; i < 1000; ++i)
        single.push_back(rolldown::rollIndex(built.table, a));

    // Check version 1: return a std::vector
    CHECK(rolldown::rollIndexes(built.table, b, 1000) == single);

    // Check version 2: updates an existing iterator
    std::vector<size_t> multiple;
    multiple.reserve(1000);
    rolldown::rollIndexes(built.table, c, 1000, std::back_inserter(multiple));
    CHECK(multiple == single);
}

TEST_CASE("the loader reads a table file", "[loader]")
{
    SECTION("load from items3.json")
    {
        const auto results = loadTablesFromJson(ROLLDOWN_TEST_DATA "/items3.json");

        REQUIRE(results.error == LoadError::None);
        const auto specs = results.tables;

        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].tiers.size() == 3);
        CHECK(specs[0].tiers[0].packs.size() == 2);
        CHECK(specs[0].tiers[0].packs[1].items.size() == 2); // "Item Axe, Item Sword" is one pack of two items
        CHECK(buildDropTable(specs[0]));
    }

    SECTION("load from itemsErrorTiersAndItemProbs.json")
    {
        const auto results = loadTablesFromJson(ROLLDOWN_TEST_DATA "/itemsErrorTiersAndItemProbs.json");

        REQUIRE(results.error == LoadError::None);

        const auto specs = results.tables;

        REQUIRE(specs.size() == 4);

        const auto specResult1 = buildDropTable(specs[0]);
        CHECK(specResult1.error == BuildError::TierSumNot100);

        const auto specResult2 = buildDropTable(specs[1]);
        CHECK(specResult2.error == BuildError::ItemSumNot100);

        const auto specResult3 = buildDropTable(specs[2]);
        CHECK(specResult3.error == BuildError::OutOfRange);

        const auto specResult4 = buildDropTable(specs[3]);
        CHECK(specResult4.error == BuildError::EmptyTier);
    }
}
