#pragma once
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>

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

// Branchless attempt
int BRANCHLESSselectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs)
{
    uint64_t roll = distrib(gen);
    size_t idx = 0;
    for (uint64_t c : cutoffs)
        idx += (roll >= c); // bool converts to 0 or 1, no if needed
    return static_cast<int>(idx);
}

// Binary Search
int BINARYselectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs)
{
    uint64_t roll = distrib(gen);
    auto it = std::upper_bound(cutoffs.begin(), cutoffs.end(), roll);
    return static_cast<int>(it - cutoffs.begin());
}