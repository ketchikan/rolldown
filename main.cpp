#include <iostream>
#include <string>
#include <numeric>
#include <fstream>
#include <cstdlib>
#include <cmath>

#include <chrono>

#include "dropChanceCalc.hpp"

// A system for selecting loot drops while preserving specific drop ratios.
// An implementation/modification to roll down loot drop methodology

constexpr int ROLLS = 100'000'000;

uint64_t calculateItemDropRate(const uint64_t &tierChance, const uint64_t &itemChance)
{
    return tierChance * itemChance;
}

uint64_t convertRateToRange(const std::string &rate)
{
    return std::llround(std::stod(rate) * PERCENT_SCALE); // "5.5" -> 550, "24.5" -> 2450
}

struct itemDetails
{
    std::string tierChance;
    std::string itemChance;

    itemDetails(std::string tierChance, std::string itemChance) : tierChance(tierChance), itemChance(itemChance) {}
};

std::vector<uint64_t> generateCutoffs(const std::vector<itemDetails> &items)
{
    std::vector<uint64_t> cutoffs;
    uint64_t currentCutoff = 0;

    for (const auto item : items)
    {
        // Convert the tierChance and itemChance
        uint64_t tierConverted = convertRateToRange(item.tierChance);
        uint64_t itemConverted = convertRateToRange(item.itemChance);

        // Calculate the cutoff
        uint64_t itemRange = calculateItemDropRate(tierConverted, itemConverted);
        currentCutoff += itemRange;
        cutoffs.push_back(currentCutoff);
    }
    return cutoffs;
}

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

    // METHOD 2
    start = std::chrono::high_resolution_clock::now();
    itemIndexes = selectNRandomDropItemIndexes(cutoffs, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 3
    start = std::chrono::high_resolution_clock::now();
    ALTselectNRandomDropItemIndexes(cutoffs, itemIndexes, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 4: BOUNDED
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ROLLS; i++)
    {
        chosenItemIdx = BOUNDEDselectRandomDropItemIndex(cutoffs);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 5
    start = std::chrono::high_resolution_clock::now();
    itemIndexes = BOUNDEDselectNRandomDropItemIndexes(cutoffs, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 6
    start = std::chrono::high_resolution_clock::now();
    BOUNDEDALTselectNRandomDropItemIndexes(cutoffs, itemIndexes, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 7
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ROLLS; i++)
    {
        chosenItemIdx = XOSHIROselectRandomDropItemIndex(cutoffs);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 8
    start = std::chrono::high_resolution_clock::now();
    itemIndexes = XOSHIROselectNRandomDropItemIndexes(cutoffs, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();

    // METHOD 9
    start = std::chrono::high_resolution_clock::now();
    XOSHIROALTselectNRandomDropItemIndexes(cutoffs, itemIndexes, ROLLS);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << duration << " microseconds\n";
    itemIndexes.clear();
}

int main()
{
    // ITEMS
    std::vector<itemDetails> items = {
        itemDetails("5.5", "5.5"),
        itemDetails("5.5", "94.5"),
        itemDetails("24.5", "60"),
        itemDetails("24.5", "40"),
        itemDetails("70", "60"),
        itemDetails("70", "40"),
    };
    std::vector<uint64_t> cutoffs = generateCutoffs(items);
    outputChances(cutoffs);
    return 0;
}