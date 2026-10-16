#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "types.hpp"

namespace rolldown
{
    // ERRORS

    /*
    @enum BuildError

    @brief Errors describing the invalid states that the defined tier or item probabilities can be found in.
    */
    enum class BuildError : uint8_t
    {
        None,
        NoTiers,
        EmptyTier,
        NegativeProbability,
        TierSumNot100,
        ItemSumNot100,
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

    @brief Convert a `BuildError` value to a string.
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
        case BuildError::NegativeProbability:
            return "A tier or item was defined with a negative probability. Please ensure all probabilities are defined as positive integers or floats between 0 - 100.";
        case BuildError::TierSumNot100:
            return "The sum of probabilities of the defined tiers is not equal to 100%.";
        case BuildError::ItemSumNot100:
            return "The sum of probabilities for items within a single tier is not equal to 100%.";
        default:
            return "Unknown error.";
        }
    }

    /*
     */
    inline uint64_t convertRateToRange(const double &rate)
    {
        // Precondition: rate >= 0 (checked by buildDropTable), so the cast can't wrap
        return static_cast<uint64_t>(std::llround(rate * PERCENT_SCALE)); // "5.5" -> 550, "24.5" -> 2450
    }

    /*
    @fn buildDropTable

    @brief Validate the structure and probabilities found in `ItemDropTable` objects and convert them to `BuildResult` objects.

    @return `BuildResult` contains the `DropTable` object as well as relevant error details to address or ignore.

    @note Important: the table returns immediately after an error is detected. This could result in empty or partially completed `DropTable` objects. Validating the output is recommended.


    @code{.cpp}
    const auto result = rolldown::buildDropTable(spec);
    if (!result)
    {
        std::cerr << rolldown::toString(result.error) << " (tier " << result.tierIndex << ")\n";
        continue;
    }
    @endcode
    */
    inline BuildResult buildDropTable(const ItemDropTable &spec)
    {
        BuildResult r;
        auto fail = [&r](BuildError error, size_t tier = 0)
        {
            r.error = error;
            r.tierIndex = tier;
            return r;
        };

        uint64_t currentCutoff = 0;
        uint64_t tierTotal = 0;

        // Validate that at least 1 tier exists
        if (spec.tiers.empty())
        {
            return fail(BuildError::NoTiers);
        }

        // Iterate over the tiers
        size_t currentTier = 0;
        for (const auto &tier : spec.tiers)
        {
            // Validate that at least 1 item pack exists
            if (tier.items.empty())
            {
                return fail(BuildError::EmptyTier, currentTier);
            }

            // Validate that tier probability is >0
            if (tier.probability < 0)
            {
                return fail(BuildError::NegativeProbability, currentTier);
            }

            // Convert tier probability
            uint64_t tierConvertedProb = convertRateToRange(tier.probability);
            tierTotal += tierConvertedProb;

            uint64_t itemProbSum = 0;
            for (const auto &pack : tier.items)
            {
                // Validate that the pack has a probability >0
                if (pack.probability < 0)
                {
                    return fail(BuildError::NegativeProbability, currentTier);
                }

                // Conver item probability
                uint64_t itemConvertedProb = convertRateToRange(pack.probability);
                itemProbSum += itemConvertedProb;

                // Calculate the cutoff
                uint64_t itemRange = tierConvertedProb * itemConvertedProb; // This used to be a function but that's dumb
                currentCutoff += itemRange;
                r.table.packs.push_back(pack);
                r.table.cutoffs.push_back(currentCutoff);
            }
            if (itemProbSum != ONE_HUNDRED_PERCENT)
            {
                return fail(BuildError::ItemSumNot100, currentTier);
            }

            currentTier++;
        }

        if (tierTotal != ONE_HUNDRED_PERCENT)
        {
            return fail(BuildError::TierSumNot100, currentTier);
        }

        return r;
    }

} // namespace rolldown
