// Example: load drop tables from JSON with nlohmann/json, then roll them.
// Copy json_loader.hpp into your project and change it to match your own data format.

#include <iostream>
#include <random>

#include <nlohmann/json.hpp>

#include "json_loader.hpp" // Found next to this file: no include path needed
#include "rolldown/rolldown.hpp"

static void rollTenTimes(const rolldown::DropTableSpec &spec, std::mt19937_64 &rng)
{
    const auto built = rolldown::buildDropTable(spec);
    if (!built)
    {
        std::cerr << "Table " << spec.id << ": " << rolldown::toString(built.error) << " (tier " << built.tierIndex << ")\n";
        return;
    }

    std::cout << "Table " << spec.id << ":";
    for (int i = 0; i < 10; ++i)
    {
        const rolldown::ItemPack &pack = rolldown::roll(built.table, rng);
        std::cout << (pack.items.empty() ? " (nothing)" : " " + pack.items[0].name);
    }
    std::cout << "\n";
}

int main()
{
    std::mt19937_64 rng{std::random_device{}()};

    // 1. From a file
    const auto fromFile = rolldown_json::loadTablesFromJson(ROLLDOWN_EXAMPLE_DIR "/loot.json");
    if (!fromFile)
    {
        std::cerr << rolldown_json::toString(fromFile.error) << " at " << fromFile.where << "\n";
        return 1;
    }
    for (const auto &spec : fromFile.tables)
        rollTenTimes(spec, rng);

    // 2. From JSON that the game already has in memory
    const auto json = nlohmann::json::parse(R"({
        "characters": [
            { "id": 99, "tiers": [ { "name": "All", "probability": 100, "packs": [
                { "probability": 75, "items": [] },
                { "probability": 25, "items": [ { "id": 1, "name": "Coin" } ] }
            ] } ] }
        ]
    })");
    const auto fromMemory = rolldown_json::parseTables(json);
    if (fromMemory)
        rollTenTimes(fromMemory.tables[0], rng);

    return 0;
}
