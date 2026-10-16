#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rolldown
{

    constexpr uint64_t PERCENT_SCALE = 100;                               // two decimal places: "5.5" -> 550
    constexpr uint64_t ONE_HUNDRED_PERCENT = 100 * PERCENT_SCALE;         // 10'000
    constexpr uint64_t TOTAL = ONE_HUNDRED_PERCENT * ONE_HUNDRED_PERCENT; // 100'000'000

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
    struct DropTier
    {
        uint64_t id;
        std::string name;
        double probability;
        std::vector<ItemPack> items;

        // Construct without ItemPack vector
        DropTier(const std::string &name, const double &probability) : id(nextId++), name(name), probability(probability) {}

        // Construct with ItemPack vector
        DropTier(const std::string &name, const double &probability, const std::vector<ItemPack> &itemPacks) : id(nextId++), name(name), probability(probability), items(itemPacks) {}

    private:
        inline static uint64_t nextId = 1;
    };

    /*
    @struct NumberDrops

    @brief Describe the number of rolls to be performed for dropping random items and the probability of that number of rolls.
     */
    struct NumberDrops
    {
        uint8_t numRolls;
        float probability;
    };

    /*
     */
    struct ItemDropTable
    {
        std::vector<NumberDrops> numDrops;
        std::vector<DropTier> tiers;
    };

    /*
    @struct DropTable

    @brief A validated table, ready to perform rolls on. Built at load time with buildDropTable.
    @note This is implemented utilizing Struct-of-arrays to help pack up to 8 cutoffs into a single 64-byte cache line.
    */
    struct DropTable
    {
        std::vector<ItemPack> packs;   // every pack from every tier, in roll order
        std::vector<uint64_t> cutoffs; // cutoffs[i] is the upper roll limit of packs[i]
    };

} // namespace rolldown
