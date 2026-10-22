#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <cassert>

#include "types.hpp"

// Determine whether MSVC is the compiler being used or not. If it is, we need an additional include.
#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
#include <intrin.h> // _umul128 / __umulh
#endif

namespace rolldown
{

    namespace detail
    {
        /*
        @fn mul128

        @brief A full 64x64 -> 128-bit multiply. Returns the low 64 bits, and writes the high 64 bits to 'high'.

        @param[in] a the first 64-bit number to be multiplied
        @param[in] b the second 64-bit number to be multiplied
        @param[out] high stores the high 64 bits of the result

        @return uint64_t the low 64 bits from the result

        @note This was written to help boundedRandom work with GCC, Clang, and MSVC, utilizing preprocessor logic to determine the compiler support for __int128 objects and adjust accordingly.
        */
        inline uint64_t mul128(uint64_t a, uint64_t b, uint64_t &high)
        {
#if defined(__SIZEOF_INT128__) // GCC, Clang, clang-cl
            const unsigned __int128 m = static_cast<unsigned __int128>(a) * b;
            high = static_cast<uint64_t>(m >> 64);
            return static_cast<uint64_t>(m);
#elif defined(_MSC_VER) && defined(_M_X64)   // MSVC on x64
            return _umul128(a, b, &high);
#elif defined(_MSC_VER) && defined(_M_ARM64) // MSVC on ARM64
            high = __umulh(a, b);
            return a * b;
#else                                        // Anything else: build it from four 32x32 multiplies
            const uint64_t aLo = a & 0xFFFFFFFF, aHi = a >> 32;
            const uint64_t bLo = b & 0xFFFFFFFF, bHi = b >> 32;
            const uint64_t ll = aLo * bLo, lh = aLo * bHi, hl = aHi * bLo, hh = aHi * bHi;
            const uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFF) + (hl & 0xFFFFFFFF);
            high = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
            return (mid << 32) | (ll & 0xFFFFFFFF);
#endif
        }

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
            uint64_t high;
            uint64_t low = mul128(x, range, high);
            if (low < range) // rare case: check for the biased region
            {
                const uint64_t threshold = (0 - range) % range;
                while (low < threshold)
                {
                    x = rng();
                    low = mul128(x, range, high);
                }
            }
            return high;
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
        assert(table.size() > 0 && "rollIndex called on an empty DropTable; check BuildResult first");

        // Get a random number from [0, TOTAL-1)
        const uint64_t roll = detail::boundedRandom(rng, detail::TOTAL);
        const std::vector<uint64_t> &cutoffs = table.cutoffs_;

        // First cutoff strictly greater than the roll is the item we landed in
        // This is now O(log n) instead of O(n)

        const uint64_t *base = cutoffs.data(); // TODO change to uint32_t
        size_t len = cutoffs.size();
        while (len > 1)
        {
            const size_t half = len / 2;
            base = (base[half - 1] <= roll) ? base + half : base; // A select instead of a branch
            len -= half;
        }
        return static_cast<size_t>(base - cutoffs.data()) + (*base <= roll);
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
