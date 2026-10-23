#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rolldown
{
    constexpr uint64_t PERCENT_SCALE = 100; // two decimal places: "5.5" -> 550

    namespace detail
    {
        constexpr uint64_t ONE_HUNDRED_PERCENT = 100 * PERCENT_SCALE;         // 10'000
        constexpr uint64_t TOTAL = ONE_HUNDRED_PERCENT * ONE_HUNDRED_PERCENT; // 100'000'000
    }

    /*
    @struct Item

    @brief Represents a single item, storing an ID and a name.
     */
    struct Item
    {
        uint64_t id; // Just generalizing for now, this should be sized like how we size ENUM's
        // std::string name;

        Item(const uint64_t id) : id(id) {}
    };

    /*
    @struct ItemPack

    @brief A collection of 1+ `Item` objects. Stores the `Item` objects in a vector, and a probability of the pack being selected.
     */
    struct ItemPack
    {
        std::vector<Item> items;
        double probability;

        // Initialize with a default probability of 100%
        ItemPack(const std::vector<Item> items) : items(std::move(items)), probability(100.0) {}

        // Initialize with a specified probability
        ItemPack(const std::vector<Item> items, const double probability) : items(std::move(items)), probability(probability) {}
    };

    /*
    @struct TierSpec

    @brief A description of a single Tier inside a drop table.

    Stores a tier ID, the name of the tier, the probability of the tier being selected, and a vector of `ItemPack` objects.
     */
    struct TierSpec
    {
        std::string name;
        double probability;
        std::vector<ItemPack> packs;

        // Construct without ItemPack vector
        TierSpec(const std::string &name, const double probability) : name(name), probability(probability) {}

        // Construct with ItemPack vector
        TierSpec(const std::string &name, const double probability, const std::vector<ItemPack> itemPacks) : name(name), probability(probability), packs(std::move(itemPacks)) {}
    };

    // /*
    // @struct NumberDrops

    // @brief Describe the number of rolls to be performed for dropping random items and the probability of that number of rolls.
    //  */
    // struct NumberDrops
    // {
    //     uint8_t numRolls;
    //     float probability;
    // };

    /*
    @struct DropTableSpec

    @brief A collection of `TierSpec` objects that comprise a single item drop table.
     */
    struct DropTableSpec
    {
        // std::vector<NumberDrops> numDrops;
        uint64_t id = 0; // The game's key for this table (for example, a character ID). 0 if the source has none.
        std::vector<TierSpec> tiers;
    };

    struct BuildResult; // forward declaration; defined in builder.hpp
    /*
    @class DropTable

    @brief A validated table, ready to roll. Only buildDropTable can create a filled one.

    @note The following rules are applied to tables from successful builds: packs_.size() == cutoffs_.size(), cutoffs_ is ascending, cutoffs_.back() == TOTAL
    */
    class DropTable
    {
    public:
        const std::vector<ItemPack> &packs() const { return packs_; }
        size_t size() const { return packs_.size(); }

    private:
        std::vector<ItemPack> packs_;   // every pack from every tier, in roll order
        std::vector<uint32_t> cutoffs_; // cutoffs_[i] is the upper roll limit of packs_[i]

        // The only code allowed to write to (or depend on) the private members
        friend BuildResult buildDropTable(const DropTableSpec &spec);

        template <class Rng>
        friend size_t rollIndex(const DropTable &table, Rng &rng);
    };

} // namespace rolldown
