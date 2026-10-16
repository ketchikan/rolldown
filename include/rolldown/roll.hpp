#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "types.hpp"

namespace rolldown
{
    // Any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)
    template <class Rng>
    uint64_t boundedRandom(Rng &rng, uint64_t range)
    {
        static_assert(Rng::min() == 0 && Rng::max() == std::numeric_limits<uint64_t>::max(),
                      "rolldown needs a generator that produces full 64-bit values, e.g. std::mt19937_64");

        // Using Lemire's method
        uint64_t x = rng();
        unsigned __int128 m = static_cast<unsigned __int128>(x) * range;
        uint64_t low = static_cast<uint64_t>(m);
        if (low < range) // rare case: check for the biased region
        {
            const uint64_t threshold = (0 - range) % range;
            while (low < threshold)
            {
                x = rng();
                m = static_cast<unsigned __int128>(x) * range;
                low = static_cast<uint64_t>(m);
            }
        }
        return static_cast<uint64_t>(m >> 64); // high 64 bits are the result
    }

    template <class Rng>
    size_t selectRandomDropItemIndex(const std::vector<uint64_t> &cutoffs, Rng &rng)
    {
        // Get a random number from [0, TOTAL-1)
        const uint64_t roll = boundedRandom(rng, TOTAL);

        // First cutoff strictly greater than the roll is the item we landed in
        // This is now O(log n) instead of O(n)
        const auto it = std::upper_bound(cutoffs.begin(), cutoffs.end(), roll);
        return static_cast<size_t>(it - cutoffs.begin());
    }

} // namespace rolldown
