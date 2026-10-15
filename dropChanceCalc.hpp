#pragma once
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>

#include "xoshiro.hpp"

// TODO Benchmark the BoundedRandom
// TODO Benchmark xoshiro256++
// TODO Benchmark PCG64

constexpr uint64_t PERCENT_SCALE = 100;                               // two decimal places: "5.5" -> 550
constexpr uint64_t ONE_HUNDRED_PERCENT = 100 * PERCENT_SCALE;         // 10'000
constexpr uint64_t TOTAL = ONE_HUNDRED_PERCENT * ONE_HUNDRED_PERCENT; // 100'000'000

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<uint64_t> distrib(0, TOTAL - 1);

// CUTOFF PROPERTIES
// The `cutoff` vector is in ascending order (lowest to highest)

int selectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs)
{
    // Get a random number from (0, TOTAL-1)
    uint64_t roll = distrib(gen);

    for (int i = 0; i < cutoffs.size(); i++)
    {
        if (roll < cutoffs.at(i))
        {
            // Return the index of the item we hit the cutoff for
            return i;
        }
    }
    // Fallback
    return cutoffs.size() - 1;
}

std::vector<int> selectNRandomDropItemIndexes(const std::vector<uint64_t> &cutoffs, const size_t &numRolls)
{
    std::vector<int> rollResults;
    uint64_t roll = 0;
    for (int i = 0; i < numRolls; ++i)
    {
        roll = distrib(gen);
        for (int j = 0; j < cutoffs.size(); j++)
        {
            if (roll < cutoffs.at(j))
            {
                // Return the index of the item we hit the cutoff for
                rollResults.emplace_back(j);
                break;
            }
        }
    }
    return rollResults;
}

void ALTselectNRandomDropItemIndexes(const std::vector<uint64_t> &cutoffs, std::vector<int> &rollResults, const size_t &numRolls)
{
    uint64_t roll = 0;
    for (int i = 0; i < numRolls; ++i)
    {
        roll = distrib(gen);
        for (int j = 0; j < cutoffs.size(); j++)
        {
            if (roll < cutoffs.at(j))
            {
                // Return the index of the item we hit the cutoff for
                rollResults.emplace_back(j);
                break;
            }
        }
    }
    return;
}

// Using Lemire's method
uint64_t boundedRandom(uint64_t range)
{
    uint64_t x = gen(); // gen must be std::mt19937_64
    unsigned __int128 m = static_cast<unsigned __int128>(x) * range;
    uint64_t low = static_cast<uint64_t>(m);
    if (low < range) // rare case: check for the biased region
    {
        const uint64_t threshold = (0 - range) % range;
        while (low < threshold)
        {
            x = gen();
            m = static_cast<unsigned __int128>(x) * range;
            low = static_cast<uint64_t>(m);
        }
    }
    return static_cast<uint64_t>(m >> 64); // high 64 bits are the result
}

int BOUNDEDselectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs)
{
    // Get a random number from (0, TOTAL-1)
    uint64_t roll = boundedRandom(TOTAL);

    for (int i = 0; i < cutoffs.size(); i++)
    {
        if (roll < cutoffs.at(i))
        {
            // Return the index of the item we hit the cutoff for
            return i;
        }
    }
    // Fallback
    return cutoffs.size() - 1;
}

std::vector<int> BOUNDEDselectNRandomDropItemIndexes(const std::vector<uint64_t> &cutoffs, const size_t &numRolls)
{
    std::vector<int> rollResults;
    uint64_t roll = 0;
    for (int i = 0; i < numRolls; ++i)
    {
        roll = boundedRandom(TOTAL);
        for (int j = 0; j < cutoffs.size(); j++)
        {
            if (roll < cutoffs.at(j))
            {
                // Return the index of the item we hit the cutoff for
                rollResults.emplace_back(j);
                break;
            }
        }
    }
    return rollResults;
}

void BOUNDEDALTselectNRandomDropItemIndexes(const std::vector<uint64_t> &cutoffs, std::vector<int> &rollResults, const size_t &numRolls)
{
    uint64_t roll = 0;
    for (int i = 0; i < numRolls; ++i)
    {
        roll = boundedRandom(TOTAL);
        for (int j = 0; j < cutoffs.size(); j++)
        {
            if (roll < cutoffs.at(j))
            {
                // Return the index of the item we hit the cutoff for
                rollResults.emplace_back(j);
                break;
            }
        }
    }
    return;
}