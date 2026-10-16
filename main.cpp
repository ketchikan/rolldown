#include <iostream>
#include <string>
#include <chrono>
#include <optional>

#include "dropChanceCalc.hpp"
#include "itemFileParse.hpp"
// #include "item.hpp"

// A system for selecting loot drops while preserving specific drop ratios.
// An implementation/modification to roll down loot drop methodology

constexpr int ROLLS = 100'000'000;

/*
 */
uint64_t convertRateToRange(const double &rate)
{
    return std::llround(rate * PERCENT_SCALE); // "5.5" -> 550, "24.5" -> 2450
}

/*
 */
std::vector<uint64_t> generateCutoffs(const ItemDropTable &table)
{
    std::vector<uint64_t> cutoffs;
    uint64_t currentCutoff = 0;
    for (const auto tier : table.tiers)
    {
        // Convert tier probability
        uint64_t tierConvertedProb = convertRateToRange(tier.probability);
        for (const auto pack : tier.items)
        {
            // Conver item probability
            uint64_t itemConvertedProb = convertRateToRange(pack.probability);

            // Calculate the cutoff
            uint64_t itemRange = tierConvertedProb * itemConvertedProb; // This used to be a function but that's dumb
            currentCutoff += itemRange;
            cutoffs.push_back(currentCutoff);
        }
    }
    return cutoffs;
}

/*
 */
void outputChances(const std::vector<uint64_t> &cutoffs)
{
    int chosenItemIdx = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::vector<int> itemIndexes;

    // METHOD 1
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ROLLS; i++)
    {
        chosenItemIdx = selectRandomDropItemIndex(cutoffs);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
}

void testCalculations(std::string_view f)
{
    std::vector<ItemDropTable> tables = testFileParsing(f);
    for (auto table : tables)
    {
        std::vector<uint64_t> cutoffs = generateCutoffs(table);
        outputChances(cutoffs);
    }
}

int main()
{
    testCalculations("tests/items1.txt");
    testCalculations("tests/items2.txt");
    testCalculations("tests/items3.txt");
    testCalculations("tests/items4.txt");

    return 0;
}