#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <limits>

#include "types.hpp"

namespace rolldown
{

    namespace detail
    {
        /*
        @fn boundedRandom

        @brief An implementation of Lemire's method of random number generation from 0 to a specified upper limit.

        @param[in] rng Any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)
        @param[in] range an integer representing the upper limit that can be generated.

        @return `uint64_t` the randomly selected integer.
         */
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
    }

    /*
    @fn rollIndex

    @brief Used to randomly select one index utilizing generated cutoffs, which follows the probability supported by the generated cutoffs.

    @param[in] table The `DropTable` object which contains the generated cutoff values
    @param[in] rng Any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)

    @return `size_t` The randomly selected item index.
    */
    template <class Rng>
    size_t rollIndex(const DropTable &table, Rng &rng)
    {
        // Get a random number from [0, TOTAL-1)
        const uint64_t roll = detail::boundedRandom(rng, detail::TOTAL);
        const std::vector<uint64_t> &cutoffs = table.cutoffs_;

        // First cutoff strictly greater than the roll is the item we landed in
        // This is now O(log n) instead of O(n)

        const auto it = std::upper_bound(cutoffs.begin(), cutoffs.end(), roll);
        return static_cast<size_t>(it - cutoffs.begin());
    }

    /*
    @fn roll

    @brief After providing a `DropTable` and a 64-bit RNG method, receive back a randomly selected `ItemPack` object from the `DropTable`.

    @param[in] table: a `DropTable` object
    @param[in] rng: any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)
    */
    template <class Rng>
    const ItemPack &roll(const DropTable &table, Rng &rng)
    {
        return table.packs()[rollIndex(table, rng)];
    }

} // namespace rolldown
