#include <catch2/catch_test_macros.hpp>

#include "rolldown/rolldown.hpp"
#include "spec_helpers.hpp"

using rolldown::buildDropTable;
using rolldown::BuildError;

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
        const auto result = buildDropTable(makeSpec({{100, {110, -10}}}));
        CHECK(result.error == BuildError::NegativeProbability);
    }
    SECTION("a negative tier probability")
    {
        const auto result = buildDropTable(makeSpec({{-100, {110, 10}}}));
        CHECK(result.error == BuildError::NegativeProbability);
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
}

TEST_CASE("the loader reads a table file", "[loader]")
{
    SECTION("load from items3.json")
    {
        const auto results = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/items3.json");

        REQUIRE(results.error == rolldown::LoadError::None);
        const auto specs = results.tables;

        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].tiers.size() == 3);
        CHECK(specs[0].tiers[0].packs.size() == 2);
        CHECK(specs[0].tiers[0].packs[1].items.size() == 2); // "Item Axe, Item Sword" is one pack of two items
        CHECK(buildDropTable(specs[0]));
    }

    SECTION("load from itemsErrorTiersAndItemProbs.json")
    {
        const auto results = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/itemsErrorTiersAndItemProbs.json");

        REQUIRE(results.error == rolldown::LoadError::None);

        const auto specs = results.tables;

        REQUIRE(specs.size() == 4);

        const auto specResult1 = buildDropTable(specs[0]);
        CHECK(specResult1.error == BuildError::TierSumNot100);

        const auto specResult2 = buildDropTable(specs[1]);
        CHECK(specResult2.error == BuildError::ItemSumNot100);

        const auto specResult3 = buildDropTable(specs[2]);
        CHECK(specResult3.error == BuildError::NegativeProbability);

        const auto specResult4 = buildDropTable(specs[3]);
        CHECK(specResult4.error == BuildError::EmptyTier);
    }
}
