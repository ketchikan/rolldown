#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "types.hpp"

namespace rolldown
{

// TODO let the caller pass in their own generator (seedable/replayable) instead of a per-file static one
static std::random_device rd;
static std::mt19937_64 gen(rd());

// Using Lemire's method
inline uint64_t boundedRandom(uint64_t range)
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

inline int selectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs)
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

} // namespace rolldown
