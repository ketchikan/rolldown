#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "types.hpp"

namespace rolldown
{

    /*
     */
    inline uint64_t convertRateToRange(const double &rate)
    {
        // TODO A negative number will turn this into a massive number. Need to validate.
        return std::llround(rate * PERCENT_SCALE); // "5.5" -> 550, "24.5" -> 2450
    }

    /*
     */
    inline std::vector<uint64_t> generateCutoffs(const ItemDropTable &table)
    {
        std::vector<uint64_t> cutoffs;
        uint64_t currentCutoff = 0;
        uint64_t tierTotal = 0;
        for (const auto &tier : table.tiers)
        {
            // Convert tier probability
            uint64_t tierConvertedProb = convertRateToRange(tier.probability);
            tierTotal += tierConvertedProb;
            for (const auto &pack : tier.items)
            {
                // Conver item probability
                uint64_t itemConvertedProb = convertRateToRange(pack.probability);

                // Calculate the cutoff
                uint64_t itemRange = tierConvertedProb * itemConvertedProb; // This used to be a function but that's dumb
                currentCutoff += itemRange;
                cutoffs.push_back(currentCutoff);
            }
        }
        if (tierTotal == ONE_HUNDRED_PERCENT && currentCutoff == TOTAL)
        {
            return cutoffs;
        }
        std::cerr << "ERROR: TIERS OR ITEMS WITHIN TIERS EXCEED A SUM OF 100%" << std::endl;
        return cutoffs;
    }

} // namespace rolldown
