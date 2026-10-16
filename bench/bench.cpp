#include <chrono>
#include <iostream>
#include <string_view>
#include <vector>
#include <random>

#include "rolldown/rolldown.hpp"

// A system for selecting loot drops while preserving specific drop ratios.
// An implementation/modification to roll down loot drop methodology

constexpr int ROLLS = 100'000'000;
// static std::random_device rd;
// static std::mt19937_64 gen(rd());

// FIXED SEED
static std::mt19937_64 rng{12345};

/*
 */
void outputChances(const std::vector<uint64_t> &cutoffs)
{
    size_t chosenItemIdx = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // RANDOMIZED
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ROLLS; i++)
    {
        // chosenItemIdx = rolldown::selectRandomDropItemIndex(cutoffs, gen);
        chosenItemIdx = rolldown::selectRandomDropItemIndex(cutoffs, rng);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";

    // STATIC
}

void testCalculations(std::string_view f)
{
    std::vector<rolldown::ItemDropTable> tables = rolldown::testFileParsing(f);
    for (auto table : tables)
    {
        std::vector<uint64_t> cutoffs = rolldown::generateCutoffs(table);
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
