#pragma once

#include <utility>
#include <vector>

#include "rolldown/rolldown.hpp"

// Build a spec from { tierPercent, { packPercent, ... } } pairs.
// Item names don't matter to the math, so every pack holds one placeholder item.
inline rolldown::DropTableSpec makeSpec(const std::vector<std::pair<double, std::vector<double>>> &tiers)
{
    rolldown::DropTableSpec spec;
    for (const auto &[tierPercent, packPercents] : tiers)
    {
        std::vector<rolldown::ItemPack> packs;
        for (double p : packPercents)
            packs.emplace_back(std::vector<rolldown::Item>{rolldown::Item("item")}, p);
        spec.tiers.emplace_back("tier", tierPercent, packs);
    }
    return spec;
}

// A single tier with `n` packs splitting 100% as evenly as 2 decimal places allow.
inline rolldown::DropTableSpec makeEvenSpec(int n)
{
    const int units = 10'000; // 100.00% in hundredths of a percent
    std::vector<double> packs(static_cast<size_t>(n), (units / n) / 100.0);
    packs[0] += (units % n) / 100.0; // leftover hundredths go to the first pack
    return makeSpec({{100.0, packs}});
}

// The chance of each pack, worked out straight from the spec: tier% x pack%.
// Deliberately NOT read from the built table: a test must not trust the code it is testing.
inline std::vector<double> expectedProbabilities(const rolldown::DropTableSpec &spec)
{
    std::vector<double> expected;
    for (const auto &tier : spec.tiers)
        for (const auto &pack : tier.packs)
            expected.push_back(tier.probability / 100.0 * pack.probability / 100.0);
    return expected;
}
