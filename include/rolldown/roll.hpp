#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <cassert>
#include <type_traits>
#include <utility>

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

        @return `uint32_t` the randomly selected integer.
         */
        template <class Rng>
        uint32_t boundedRandom(Rng &rng, uint32_t range)
        {
            static_assert(Rng::min() == 0 && Rng::max() == std::numeric_limits<uint64_t>::max(),
                          "rolldown needs a generator that produces full 64-bit values, e.g. std::mt19937_64");

            uint64_t m = (rng() >> 32) * uint64_t{range}; // top 32 random bits x range
            uint32_t low = static_cast<uint32_t>(m);
            if (low < range) // ~2% of rolls: the result might be in the biased region
            {
                const uint32_t threshold = (0u - range) % range; // = 2^32 mod range: the values to reject
                while (low < threshold)
                {
                    m = (rng() >> 32) * uint64_t{range};
                    low = static_cast<uint32_t>(m);
                }
            }
            return static_cast<uint32_t>(m >> 32);
        }

        // ITERATOR CHECKING
        // True if `*out++ = size_t` compiles for It: exactly what rollIndexes does with it.
        template <class It, class = void>
        struct acceptsIndexes : std::false_type
        {
        };

        template <class It>
        struct acceptsIndexes<It, std::void_t<decltype(*std::declval<It &>()++ = std::declval<std::size_t>())>> : std::true_type
        {
        };
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
        const uint32_t roll = detail::boundedRandom(rng, detail::TOTAL);
        const std::vector<uint32_t> &cutoffs = table.cutoffs_;

        // First cutoff strictly greater than the roll is the item we landed in
        // This is now O(log n) instead of O(n)

        const uint32_t *base = cutoffs.data();
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

    @return `ItemPack`
    */
    template <class Rng>
    const ItemPack &roll(const DropTable &table, Rng &rng)
    {
        return table.packs()[rollIndex(table, rng)];
    }

    /*
    @fn rollIndexes

    @brief Roll `count` indexes and write them through `out`. No allocation: reuse your own buffer.

    @param[in] table: a `DropTable` object
    @param[in] rng: any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)
    @param[in] count: the number of indexes to randomly select.
    @param[out] out Where to write the indexes. Must have room for `count` values, or grow as needed (for example, std::back_inserter).


    @return Outputs the `out` iterator one past the last write.

    @code{.cpp}
    buffer.clear(); // reuse a vector each time
    rolldown::rollIndexes(table, rng, 30, std::back_inserter(buffer)); // no allocation once it has grown

    size_t fixed[16];
    rolldown::rollIndexes(table, rng, 16, fixed); // a plain array, no heap at all
    @endcode

    See rollIndexes(table, rng, count) to get a new std::vector instead.
    */
    template <class Rng, class OutputIt>
    OutputIt rollIndexes(const DropTable &table, Rng &rng, size_t count, OutputIt out)
    {
        static_assert(detail::acceptsIndexes<OutputIt>::value,
                      "rollIndexes: `out` must be an output iterator that accepts size_t, "
                      "for example a pointer, container.begin(), or std::back_inserter(container)");

        for (size_t i = 0; i < count; ++i)
            *out++ = rollIndex(table, rng);
        return out;
    }

    /*
    @fn rollIndexes

    @brief Roll `count` indexes into a new std::vector.

    @param[in] table: a `DropTable` object
    @param[in] rng: any generator producing full 64-bit values works (std::mt19937_64, PCG64, xoshiro256**...)
    @param[in] count: the number of indexes to randomly select.

    @return `std::vector<size_t>` a vector of the randomly selected indexes

    @code{.cpp}
    auto picks = rolldown::rollIndexes(table, rng, 500); // Returns a new vector
    @endcode

    See rollIndexes(table, rng, count, out) to write into your own buffer with no allocation.
    */
    template <class Rng>
    std::vector<size_t> rollIndexes(const DropTable &table, Rng &rng, size_t count)
    {
        std::vector<size_t> result(count);
        rollIndexes(table, rng, count, result.begin());
        return result;
    }

} // namespace rolldown
