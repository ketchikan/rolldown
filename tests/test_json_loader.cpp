#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "rolldown/json_loader.hpp"
#include "rolldown/rolldown.hpp"

using rolldown::LoadError;
using rolldown::parseTables;

static std::vector<std::string> sortedNames(const rolldown::ItemPack &pack)
{
    std::vector<std::string> names;
    for (const auto &item : pack.items)
        names.push_back(item.name);
    std::sort(names.begin(), names.end());
    return names;
}

TEST_CASE("the JSON loader reads a table file", "[json]")
{
    const auto loaded = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/items3.json");

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
    CHECK(spec.tiers[0].packs[1].items[1].name == "Axe");
    CHECK(rolldown::buildDropTable(spec));
}

// The text files are the reference: each JSON file must describe the same tables.
TEST_CASE("the JSON and text loaders agree", "[json]")
{
    for (const std::string name : {"items1", "items2", "items3", "items4", "itemsCompensatingTiers"})
    {
        DYNAMIC_SECTION(name)
        {
            const auto fromText = rolldown::loadTables(ROLLDOWN_TEST_DATA "/" + name + ".txt");
            const auto fromJson = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/" + name + ".json");
            REQUIRE(fromJson);
            REQUIRE(fromJson.tables.size() == fromText.size());

            for (size_t c = 0; c < fromText.size(); ++c)
            {
                const auto &a = fromText[c];
                const auto &b = fromJson.tables[c];
                REQUIRE(a.tiers.size() == b.tiers.size());
                for (size_t t = 0; t < a.tiers.size(); ++t)
                {
                    INFO("character " << c << ", tier " << t);
                    CHECK(a.tiers[t].probability == b.tiers[t].probability);
                    REQUIRE(a.tiers[t].packs.size() == b.tiers[t].packs.size());
                    for (size_t p = 0; p < a.tiers[t].packs.size(); ++p)
                    {
                        const auto &packA = a.tiers[t].packs[p];
                        const auto &packB = b.tiers[t].packs[p];
                        INFO("pack " << p);
                        CHECK(packA.probability == packB.probability);
                        // A pack drops all its items together, so their order does not matter.
                        // Compare names only: text-loader IDs come from a counter.
                        CHECK(sortedNames(packA) == sortedNames(packB));
                    }
                }
            }
        }
    }
}

TEST_CASE("JSON problems report the error and where it is", "[json]")
{
    SECTION("the file does not exist")
    {
        const auto loaded = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/does_not_exist.json");
        CHECK(loaded.error == LoadError::FileNotFound);
    }
    SECTION("the text is not JSON")
    {
        const auto loaded = rolldown::loadTablesFromJson(ROLLDOWN_TEST_DATA "/invalid.json");
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
