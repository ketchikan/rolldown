#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "rolldown/rolldown.hpp"
#include "json_loader.hpp"

using rolldown_json::LoadError;
using rolldown_json::loadTablesFromJson;
using rolldown_json::parseTables;

TEST_CASE("the JSON loader reads a table file", "[json]")
{
    const auto loaded = loadTablesFromJson(ROLLDOWN_TEST_DATA "/items3.json");

    REQUIRE(loaded);
    REQUIRE(loaded.tables.size() == 1);
    const auto &spec = loaded.tables[0];
    CHECK(spec.id == 1);
    REQUIRE(spec.tiers.size() == 3);
    CHECK(spec.tiers[0].name == "Legendary");
    CHECK(spec.tiers[0].probability == 3.5);
    REQUIRE(spec.tiers[0].packs.size() == 2);
    REQUIRE(spec.tiers[0].packs[1].items.size() == 2); // Sword + Axe in one pack
    CHECK(spec.tiers[0].packs[1].items[1].id == 2);
    // CHECK(spec.tiers[0].packs[1].items[1].name == "Axe");
    CHECK(rolldown::buildDropTable(spec));
}

TEST_CASE("JSON problems report the error and where it is", "[json]")
{
    SECTION("the file does not exist")
    {
        const auto loaded = loadTablesFromJson(ROLLDOWN_TEST_DATA "/does_not_exist.json");
        CHECK(loaded.error == LoadError::FileNotFound);
    }
    SECTION("the text is not JSON")
    {
        const auto loaded = loadTablesFromJson(ROLLDOWN_TEST_DATA "/invalid.json");
        CHECK(loaded.error == LoadError::InvalidJson);
    }
    SECTION("no characters array")
    {
        const auto loaded = parseTables(nlohmann::json::parse(R"({ "tables": [] })"));
        CHECK(loaded.error == LoadError::MissingField);
        CHECK(loaded.where == "/characters");
    }
    SECTION("a missing field deep inside")
    {
        const auto loaded = parseTables(nlohmann::json::parse(R"({ "characters": [
            { "id": 1, "tiers": [
                { "name": "Common", "probability": 100, "packs": [
                    { "items": [] }
                ] }
            ] }
        ] })"));
        CHECK(loaded.error == LoadError::MissingField);
        CHECK(loaded.where == "/characters/0/tiers/0/packs/0/probability");
        CHECK(loaded.tables.empty());
    }
    SECTION("a number written as a string")
    {
        const auto loaded = parseTables(nlohmann::json::parse(R"({ "characters": [
            { "id": 1, "tiers": [ { "name": "Common", "probability": "100", "packs": [] } ] }
        ] })"));
        CHECK(loaded.error == LoadError::WrongType);
        CHECK(loaded.where == "/characters/0/tiers/0/probability");
    }
    SECTION("a negative ID")
    {
        const auto loaded = parseTables(nlohmann::json::parse(R"({ "characters": [ { "id": -1, "tiers": [] } ] })"));
        CHECK(loaded.error == LoadError::WrongType);
        CHECK(loaded.where == "/characters/0/id");
    }
    SECTION("an array element that is not an object")
    {
        const auto loaded = parseTables(nlohmann::json::parse(R"({ "characters": [ 42 ] })"));
        CHECK(loaded.error == LoadError::WrongType);
        CHECK(loaded.where == "/characters/0");
    }
}

TEST_CASE("an empty items array is a 'nothing drops' pack", "[json]")
{
    const auto loaded = parseTables(nlohmann::json::parse(R"({ "characters": [
        { "id": 7, "tiers": [ { "name": "All", "probability": 100, "packs": [
            { "probability": 90, "items": [] },
            { "probability": 10, "items": [ { "id": 1, "name": "Sword" } ] }
        ] } ] }
    ] })"));

    REQUIRE(loaded);
    const auto built = rolldown::buildDropTable(loaded.tables[0]);
    REQUIRE(built);
    CHECK(built.table.packs()[0].items.empty());
}
