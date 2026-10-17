#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

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
        inline uint64_t convertRateToRange(const double &rate)
        {
            // Precondition: rate >= 0 (checked by buildDropTable), so the cast can't wrap
            return static_cast<uint64_t>(std::llround(rate * PERCENT_SCALE));
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
        case BuildError::NegativeProbability:
            return "A tier or item was defined with a negative probability. Please ensure all probabilities are defined as positive integers or floats between 0 - 100.";
        case BuildError::TierSumNot100:
            return "The sum of probabilities of the defined tiers is not equal to 100%.";
        case BuildError::ItemSumNot100:
            return "The sum of probabilities for items within a single tier is not equal to 100%.";
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
        auto fail = [&r](BuildError error, size_t tier = 0)
        {
            r.table.packs_.clear();
            r.table.cutoffs_.clear();
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
        for (size_t t = 0; t < spec.tiers.size(); ++t)
        {
            const TierSpec &tier = spec.tiers[t];
            // Validate that at least 1 item pack exists
            if (tier.packs.empty())
            {
                return fail(BuildError::EmptyTier, t);
            }

            // Validate that tier probability is >0
            if (tier.probability < 0)
            {
                return fail(BuildError::NegativeProbability, t);
            }

            // Convert tier probability
            uint64_t tierConvertedProb = detail::convertRateToRange(tier.probability);
            tierTotal += tierConvertedProb;

            uint64_t itemProbSum = 0;
            for (const auto &pack : tier.packs)
            {
                // Validate that the pack has a probability >0
                if (pack.probability < 0)
                {
                    return fail(BuildError::NegativeProbability, t);
                }

                // Conver item probability
                uint64_t itemConvertedProb = detail::convertRateToRange(pack.probability);
                itemProbSum += itemConvertedProb;

                // Calculate the cutoff
                uint64_t itemRange = tierConvertedProb * itemConvertedProb; // This used to be a function but that's dumb
                currentCutoff += itemRange;
                r.table.packs_.push_back(pack);
                r.table.cutoffs_.push_back(currentCutoff);
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
