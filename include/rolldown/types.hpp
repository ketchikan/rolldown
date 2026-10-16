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

    // TODO for now I'm making these Vectors, but that is going to be very inefficient to use at scale.
    // - If I determine a maximum size for each of these, I can declare those sizes and use that instead in a cstyle array or a c++ style array

    /*
     */
    struct Item
    {
        uint64_t id; // Just generalizing for now, this should be sized like how we size ENUM's
        std::string name;

        Item(const std::string_view &name) : id(nextId++), name(name) {}

    private:
        inline static uint64_t nextId = 1;
    };

    /*
     */
    struct ItemPack
    {
        std::vector<Item> items;
        double probability;

        // Initialize with a default probability of 100%
        ItemPack(const std::vector<Item> &items) : items(items), probability(100.0) {}

        // Initialize with a specified probability
        ItemPack(const std::vector<Item> &items, const double &probability) : items(items), probability(probability) {}
    };

    /*
     */
    struct TierSpec
    {
        uint64_t id;
        std::string name;
        double probability;
        std::vector<ItemPack> packs;

        // Construct without ItemPack vector
        TierSpec(const std::string &name, const double &probability) : id(nextId++), name(name), probability(probability) {}

        // Construct with ItemPack vector
        TierSpec(const std::string &name, const double &probability, const std::vector<ItemPack> &itemPacks) : id(nextId++), name(name), probability(probability), packs(itemPacks) {}

    private:
        inline static uint64_t nextId = 1;
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
     */
    struct DropTableSpec
    {
        // std::vector<NumberDrops> numDrops;
        std::vector<TierSpec> tiers;
    };

    struct BuildResult; // forward declaration; defined in builder.hpp
    /*
    @class DropTable
    @brief A validated table, ready to roll. Only buildDropTable can create a filled one.
    @invariant packs_.size() == cutoffs_.size(), cutoffs_ is ascending, cutoffs_.back() == TOTAL
    */
    class DropTable
    {
    public:
        const std::vector<ItemPack> &packs() const { return packs_; }
        size_t size() const { return packs_.size(); }

    private:
        std::vector<ItemPack> packs_;   // every pack from every tier, in roll order
        std::vector<uint64_t> cutoffs_; // cutoffs_[i] is the upper roll limit of packs_[i]

        // The only code allowed to write to (or depend on) the private members
        friend BuildResult buildDropTable(const DropTableSpec &spec);

        template <class Rng>
        friend size_t rollIndex(const DropTable &table, Rng &rng);
    };

} // namespace rolldown
