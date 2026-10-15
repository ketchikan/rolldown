#include <iostream>
#include <string>
#include <numeric>

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

// OPTIONAL PITY SYSTEM???
// A pity system raises the chance after each miss.
// Set a maximum number of misses after which the drop is guaranteed
// If these exist, then I can publish the pity rules alongside the drop rates.

constexpr uint64_t TOTAL = 1'000'000;
constexpr uint64_t TOTAL_PERCENT_WEIGHT = TOTAL / 100; // Should be 10,000 for a total of 1'000'000

uint64_t calculateItemDropRate(const uint64_t &tierChance, const uint64_t &itemChance)
{
    return tierChance * itemChance / 100; // Should always be 100
}

uint64_t convertRateToRange(const std::string &rate)
{
    return std::stof(rate) * TOTAL_PERCENT_WEIGHT;
}

void outputChances()
{

    std::string tier1Chance = "10";
    std::string tier2Chance = "20";
    std::string tier3Chance = "70";

    std::string tier1Item1 = "60";
    std::string tier1Item2 = "40";

    std::string tier2Item1 = "60";
    std::string tier2Item2 = "40";

    std::string tier3Item1 = "60";
    std::string tier3Item2 = "40";

    // Convert Tier 1 chance into the integer
    uint64_t tier1ChanceConverted = convertRateToRange(tier1Chance); // Should be 100,000
    uint64_t tier2ChanceConverted = convertRateToRange(tier2Chance); // Should be 100,000
    uint64_t tier3ChanceConverted = convertRateToRange(tier3Chance); // Should be 100,000

    // Convert each of the tier items
    uint64_t tier1Item1Converted = std::stoi(tier1Item1);
    uint64_t tier1Item2Converted = std::stoi(tier1Item2);
    uint64_t tier2Item1Converted = std::stoi(tier2Item1);
    uint64_t tier2Item2Converted = std::stoi(tier2Item2);
    uint64_t tier3Item1Converted = std::stoi(tier3Item1);
    uint64_t tier3Item2Converted = std::stoi(tier3Item2);

    uint64_t tier1Item1Range = calculateItemDropRate(tier1ChanceConverted, tier1Item1Converted);
    uint64_t tier1Item2Range = calculateItemDropRate(tier1ChanceConverted, tier1Item2Converted);
    uint64_t tier2Item1Range = calculateItemDropRate(tier2ChanceConverted, tier2Item1Converted);
    uint64_t tier2Item2Range = calculateItemDropRate(tier2ChanceConverted, tier2Item2Converted);
    uint64_t tier3Item1Range = calculateItemDropRate(tier3ChanceConverted, tier3Item1Converted);
    uint64_t tier3Item2Range = calculateItemDropRate(tier3ChanceConverted, tier3Item2Converted);

    std::cout << "Item number 1 drop chance: " << tier1Item1Range << "%" << std::endl;
    std::cout << "Item number 2 drop chance: " << tier1Item2Range << "%" << std::endl;
    std::cout << "Item number 3 drop chance: " << tier2Item1Range << "%" << std::endl;
    std::cout << "Item number 4 drop chance: " << tier2Item2Range << "%" << std::endl;
    std::cout << "Item number 5 drop chance: " << tier3Item1Range << "%" << std::endl;
    std::cout << "Item number 6 drop chance: " << tier3Item2Range << "%" << std::endl;

    std::cout << "Total: " << tier1Item1Range + tier1Item2Range + tier2Item1Range + tier2Item2Range + tier3Item1Range + tier3Item2Range << std::endl;
}

int main()
{
    outputChances();
    return 0;
}