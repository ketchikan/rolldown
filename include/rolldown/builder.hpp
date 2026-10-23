#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <cassert>

#include "types.hpp"

namespace rolldown
{
    namespace detail
    {
        /*
        @fn convertRateToRange

        @brief Converting a double to a scaled integer. Ex: 5.5 -> 550, 24.5 -> 2450

        @param[in] rate: a double representing the rate to be converted.

        @return uint64_t an integer representing the scaled input.
        */
        inline uint32_t convertRateToRange(double rate)
        {
            // Precondition: 0 <= rate <= 100 (checked by buildDropTable), so the result is 0..10'000 and fits
            assert(rate >= 0.0 && rate <= 100.0);
            return static_cast<uint32_t>(std::llround(rate * PERCENT_SCALE));
        }
    }

    /*
    @enum BuildError

    @brief Errors describing the invalid states that the defined tier or item probabilities can be found in.
    */
    enum class BuildError : uint8_t
    {
        None,
        NoTiers,
        EmptyTier,
        TierSumNot100,
        ItemSumNot100,
        OutOfRange,
        TooPrecise,
    };

    /*
    @struct BuildResult

    @brief Contains the `DropTable` and relevant errors resulting from a build attempt of a table.
    */
    struct BuildResult
    {
        DropTable table;
        BuildError error = BuildError::None;
        size_t tierIndex = 0; // Which tier caused the error (unused for NoTiers & TierSumNot100)

        // Allows a caller to write `if (!result)`. Can't be used like `result + 1` due to explicit.
        explicit operator bool() const { return error == BuildError::None; }
    };

    /*
    @fn toString

    @brief Convert a `BuildError` value to a string to output error messages.

    @param[in] e a `BuildError` enum value.

    @return `char*` a pointer to a C-style string.
    */
    inline const char *toString(BuildError e)
    {
        switch (e)
        {
        case BuildError::None:
            return "No Error";
        case BuildError::NoTiers:
            return "No Tiers defined; please include at least 1 tier holding at least 1 item.";
        case BuildError::EmptyTier:
            return "A tier was identified that held no Item. Please ensure all tiers hold at least 1 item.";
        case BuildError::TierSumNot100:
            return "The sum of probabilities of the defined tiers is not equal to 100%.";
        case BuildError::ItemSumNot100:
            return "The sum of probabilities for items within a single tier is not equal to 100%.";
        case BuildError::OutOfRange:
            return "The tier or item was defined with a probability outside the range of (0,100).";
        case BuildError::TooPrecise:
            return "The tier or item was defined with more than two decimal places of precision.";
        }
        return "Unknown error."; // Moved out of the default because if I later add a new error type to the enum then it will throw an error through the compiler, which is handy.
    }

    /*
    @fn buildDropTable

    @brief Validate the structure and probabilities found in `DropTableSpec` objects and convert them to `BuildResult` objects.

    @return `BuildResult` contains the `DropTable` object as well as relevant error details to address or ignore.

    @note Important: Upon failure, the `DropTable` object found at BuildResult.table will contain empty vectors.


    @code{.cpp}
    const auto result = rolldown::buildDropTable(spec);
    if (!result)
    {
        std::cerr << rolldown::toString(result.error) << " (tier " << result.tierIndex << ")\n";
        continue;
    }
    @endcode
    */
    inline BuildResult buildDropTable(const DropTableSpec &spec)
    {
        BuildResult r;

        // LAMBDAS
        auto fail = [&r](BuildError error, size_t tier = 0)
        {
            r.table.packs_.clear();
            r.table.cutoffs_.clear();
            r.error = error;
            r.tierIndex = tier;
            return r;
        };

        uint64_t currentCutoff = 0;
        uint32_t tierTotal = 0;

        // Validate that at least 1 tier exists
        if (spec.tiers.empty())
        {
            return fail(BuildError::NoTiers);
        }

        // Reserve vector space before we build
        size_t numPacks = 0;
        for (const auto &tier : spec.tiers)
        {
            numPacks += tier.packs.size();
        }
        r.table.packs_.reserve(numPacks);
        r.table.cutoffs_.reserve(numPacks);

        // Iterate over the tiers
        for (size_t t = 0; t < spec.tiers.size(); ++t)
        {
            const TierSpec &tier = spec.tiers[t];
            // Validate that at least 1 item pack exists
            if (tier.packs.empty())
            {
                return fail(BuildError::EmptyTier, t);
            }

            // Validate that the probability is within the range
            if (!(tier.probability >= 0.0 && tier.probability <= 100.0)) // written this way so NaN fails too (every comparison with NaN is false)
            {
                return fail(BuildError::OutOfRange, t);
            }

            // Validate if the defined probability is too precise
            if (std::abs(tier.probability * PERCENT_SCALE - std::round(tier.probability * PERCENT_SCALE)) > 1e-6)
            {
                return fail(BuildError::TooPrecise, t);
            }

            // Convert tier probability
            uint32_t tierConvertedProb = detail::convertRateToRange(tier.probability);
            tierTotal += tierConvertedProb;

            uint32_t itemProbSum = 0;
            for (const auto &pack : tier.packs)
            {
                // Validate that the probability is within the range
                if (!(pack.probability >= 0.0 && pack.probability <= 100.0)) // written this way so NaN fails too (every comparison with NaN is false)
                {
                    return fail(BuildError::OutOfRange, t);
                }

                // Validate if the defined probability is too precise
                if (std::abs(pack.probability * PERCENT_SCALE - std::round(pack.probability * PERCENT_SCALE)) > 1e-6)
                {
                    return fail(BuildError::TooPrecise, t);
                }

                // Conver item probability
                uint32_t itemConvertedProb = detail::convertRateToRange(pack.probability);
                itemProbSum += itemConvertedProb;

                // Calculate the cutoff
                uint32_t itemRange = tierConvertedProb * itemConvertedProb; // This used to be a function but that's dumb
                currentCutoff += itemRange;
                r.table.packs_.push_back(pack);
                r.table.cutoffs_.push_back(static_cast<uint32_t>(currentCutoff));
            }
            if (itemProbSum != detail::ONE_HUNDRED_PERCENT)
            {
                return fail(BuildError::ItemSumNot100, t);
            }
        }

        if (tierTotal != detail::ONE_HUNDRED_PERCENT)
        {
            return fail(BuildError::TierSumNot100);
        }

        return r;
    }

} // namespace rolldown
