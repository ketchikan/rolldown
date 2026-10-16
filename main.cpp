#include <iostream>
#include <string>
#include <chrono>
#include <optional>

#include "dropChanceBuilder.hpp"
#include "dropChanceCalc.hpp"
#include "itemFileParse.hpp"
// #include "item.hpp"

// A system for selecting loot drops while preserving specific drop ratios.
// An implementation/modification to roll down loot drop methodology

constexpr int ROLLS = 100'000'000;

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
    testCalculations("tests/itemsErrorTiersAndItemProbs.txt");

    return 0;
}