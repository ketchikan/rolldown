#include <iostream>
#include <string>
#include <numeric>
#include <fstream>
#include <cstdlib>
#include <cmath>

#include "dropChanceCalc.hpp"

// A system for selecting loot drops while preserving specific drop ratios.
// An implementation/modification to roll down loot drop methodology

// TIER
// A group of items with the same 'rarity'
// WEIGHT
// An integer that sets the chance of a tier or item
// TOTAL
// The sum of all tier weights. This must sum to 100% of the domain.
// ROLL
// A random integer from 0 to Total minus 1
// CUTOFF
// The upper limit of a tier or item in the cumulative list
// SHARE
// An integer that sets the chance of an item inside its tier.

// Use only integers for weights and cutoffs to avoid floating point problems
// Set Total to something like 1,000,000 (this should give us plenty of precision)
// Convert percentage to weight: weight = percent * 10,000
// All tier weights should add to Total
// All item shares in each tier have a sum greater than 0
// Roll on the server only (for single player you will roll on the device)

// INTERFACE WITH A CONFIG FILE
// Some file will be defined something like this:
/*
tier Legendary 3.5
item Sword 60
item Axe 40
tier Rare 20
item Bow 50
item Shield 50
tier Common 76.5
item Potion 100
*/

// Read or parse these values from the config
// Convert each tier percent to a tier weight
// Add all tier weights. If the sum is not equal to Total, stop and show an error
// - This error should include how far off you are from Total, or the percentage adjustment you would need to make to one or more tiers
// For each item calculate:
// - item weight = tier_weight * item_share / sum of shares in the tier
// - if the item weight is not an integer, stop and show an error
// Make a list of all items in order, from rarest to most common
// Calculate the cumulative cutoff for each item
// Store the list

// LATER I WANT TO BE ABLE TO GRAB THE LIST from some arbitrary binary format, like attached to an enemy type, and have it perform the calculation quickly.
// TODO Is there a way to do this calculation branchless? That's the end goal is to do it with as few comparisons as possible, which should be good for performance and inlining the code.
// TODO I want to turn all divides into multiplications where possible
// TODO Instead of total being something like 1 million, I want to see if I can get it to a power of 2 for easier bit shifting math operations.

// Roll procedure
// Get a random integer from 0 to Total - 1
// Find the first item where roll < cutoff
// Give that item(s) to the player.

// ! DO NOT USE MODULO as that will favor smaller numbers
// Instead maybe bitmasking?
// TODO I can also use a bounded-integer function from the randomization library

// TODO CHECKS TO PERFORM (when compiling or rolling)
// The tier weights add up to Total
// Each item weight is an integer
// Each tier has at least 1 item (or item group)
// No weight is negative
// Then maybe I test by rolling some large n of rolls (say 10mn), count the drops for each and compare the observed rate to the configured rate.
// If there is a large difference (define some std deviation I think), then we need to identify the root cause before we will allow it to pass the tests and compile.

// This approach allows me to publish the odds for individual items.
// TODO If I define the odds for individual items, should I be able to have the program organize them into the 'tiers' for me?
// TODO For a game like Elden Ring, do I even need tiers?

// TODO I just realized that there is the most important item of all; no item drop.
// I need to figure out some way to determine if an item drops at all. I can easily do this by either not defining drops for an enemy, in which case we would return some value noting that they don't drop anything, or I define a NULL option of some kind so that there is a chance that we don't drop an item.

// OPTIONAL PITY SYSTEM???
// A pity system raises the chance after each miss.
// Set a maximum number of misses after which the drop is guaranteed
// If these exist, then I can publish the pity rules alongside the drop rates.
// ! A pity system would require that I keep track of the number of 'attempts', like enemies killed or rolls made
// - Because no item drop may also be an option, we want to define whether going off enemies killed or the number of items obtained from a solider will work.
// - I would also want to either come up with these values myself or come up with some kind of 'pity policy': an equation to derive the number of rolls until I want to guarantee that a drop will occur if it has no already been obtained (or is not in inventory or something)

// IMPLEMENTATION NOTES
// - I can either store the percentages as the converted numbers in the txt files or do the conversion when we are saving the games. I think I would rather do the conversions in the program to make it easier to read and bug fix if it comes down to it.

constexpr uint64_t PERCENT_SCALE = 100;                               // two decimal places: "5.5" -> 550
constexpr uint64_t ONE_HUNDRED_PERCENT = 100 * PERCENT_SCALE;         // 10'000
constexpr uint64_t TOTAL = ONE_HUNDRED_PERCENT * ONE_HUNDRED_PERCENT; // 100'000'000

constexpr int ROLLS = 1'000'000;

uint64_t calculateItemDropRate(const uint64_t &tierChance, const uint64_t &itemChance)
{
    return tierChance * itemChance;
}

uint64_t convertRateToRange(const std::string &rate)
{
    return std::llround(std::stod(rate) * PERCENT_SCALE); // "5.5" -> 550, "24.5" -> 2450
}

void outputChances()
{
    std::string tier1Chance = "5.5";
    std::string tier2Chance = "24.5";
    std::string tier3Chance = "70";

    std::string tier1Item1 = "5.5";
    std::string tier1Item2 = "94.5";

    std::string tier2Item1 = "60";
    std::string tier2Item2 = "40";

    std::string tier3Item1 = "60";
    std::string tier3Item2 = "40";

    // Convert Tier 1 chance into the integer
    uint64_t tier1ChanceConverted = convertRateToRange(tier1Chance);
    uint64_t tier2ChanceConverted = convertRateToRange(tier2Chance);
    uint64_t tier3ChanceConverted = convertRateToRange(tier3Chance);

    // Convert each of the tier items (same scale as the tiers: "5.5" -> 550)
    uint64_t tier1Item1Converted = convertRateToRange(tier1Item1);
    uint64_t tier1Item2Converted = convertRateToRange(tier1Item2);
    uint64_t tier2Item1Converted = convertRateToRange(tier2Item1);
    uint64_t tier2Item2Converted = convertRateToRange(tier2Item2);
    uint64_t tier3Item1Converted = convertRateToRange(tier3Item1);
    uint64_t tier3Item2Converted = convertRateToRange(tier3Item2);

    uint64_t tier1Item1Range = calculateItemDropRate(tier1ChanceConverted, tier1Item1Converted);
    uint64_t tier1Item2Range = calculateItemDropRate(tier1ChanceConverted, tier1Item2Converted);
    uint64_t tier2Item1Range = calculateItemDropRate(tier2ChanceConverted, tier2Item1Converted);
    uint64_t tier2Item2Range = calculateItemDropRate(tier2ChanceConverted, tier2Item2Converted);
    uint64_t tier3Item1Range = calculateItemDropRate(tier3ChanceConverted, tier3Item1Converted);
    uint64_t tier3Item2Range = calculateItemDropRate(tier3ChanceConverted, tier3Item2Converted);

    // Now I need some way of noting the correct cutoff values per item.
    // CUTOFF: Starting from the highest chance to the lowest, I need to keep track of the current running total. The cutoff value will add up all the item range values as it goes along.
    uint64_t currentCutoff = 0;
    std::vector<uint64_t> cutoffs;
    std::vector<size_t> count;

    currentCutoff += tier1Item1Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    currentCutoff += tier1Item2Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    currentCutoff += tier2Item1Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    currentCutoff += tier2Item2Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    currentCutoff += tier3Item1Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    currentCutoff += tier3Item2Range;
    cutoffs.emplace_back(currentCutoff);
    count.emplace_back(0);

    std::cout << currentCutoff << std::endl;

    std::cout << "Total: " << currentCutoff << std::endl;

    int chosenItemIdx = 0;
    for (int i = 0; i < ROLLS; i++)
    {
        chosenItemIdx = selectRandomDropItemIndex(cutoffs, TOTAL);
        count.at(chosenItemIdx) += 1;
    }

    uint64_t previousCutoff = 0;
    for (size_t i = 0; i < count.size(); i++)
    {
        // The item's slice of the range is the gap between its cutoff and the previous one
        const uint64_t itemRange = cutoffs.at(i) - previousCutoff;
        previousCutoff = cutoffs.at(i);

        const double observed = 100.0 * static_cast<double>(count.at(i)) / ROLLS;
        const double expected = 100.0 * static_cast<double>(itemRange) / TOTAL;

        std::cout << std::fixed << std::setprecision(4)
                  << "Item " << i << " chosen: " << count.at(i)
                  << ", Observed: " << observed << "%"
                  << ", Expected: " << expected << "%"
                  << std::endl;
    }
}

std::ifstream getFileStream(const std::string_view &filepath)
{
    // Open the file
    std::ifstream in(filepath);
    if (!in)
    {
        std::cerr << "FILE NOT FOUND: " << filepath << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return in;
}

void parseCharacter(std::ifstream &fs)
{
    // Iterate through the file in 'scopes'.
    // Each scope starts with a Character tag and finishes with a RBRACE
    std::string line;
    while (std::getline(fs, line))
    {
        // TODO VALIDATE INPUT AS WE GO
        // Starting a CHARACTER scope. Validate.
        if (!line.starts_with("Character"))
        {
            // Bad formatting: Doesn't start with Character
        }
    }
    // End of the file or wrong format
}

void closeFileStream(std::ifstream &fs)
{
    fs.close();
    return;
}

// I don't know why it took me so long to realize but this is technically a good use case for JSON. Hmm. I'm going to ignore that, this is more fun anyways.

int main()
{
    std::ifstream fs = getFileStream("tests/items1.txt");
    // TODO Do things
    closeFileStream(fs);

    outputChances();
    return 0;
}