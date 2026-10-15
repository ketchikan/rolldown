#pragma once
#include <vector>
#include <random>
#include <cstdint>

static std::random_device rd;
static std::mt19937 gen(rd());

int selectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs, const uint64_t &TOTAL)
{
    // Get a random number from (0, TOTAL-1)
    std::uniform_int_distribution<uint64_t> distrib(0, TOTAL - 1);
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