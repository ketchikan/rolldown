#pragma once

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "types.hpp"

// TODO Convert this to JSON based loading, which is much more common in game dev.
// TODO I should also write about how you just need to get it in the correct structure then you're set

namespace rolldown
{
    namespace detail
    {
        class ItemFile
        {
        public:
            explicit ItemFile(std::string_view fileName)
                : stream(std::string(fileName))
            {
                if (!stream)
                    throw std::runtime_error("FILE NOT FOUND: " + std::string(fileName));
            }

            // Reads the next line; returns false at end of file
            bool readLine(std::string &line)
            {
                return static_cast<bool>(std::getline(stream, line));
            }

        private:
            std::ifstream stream;
        };
    }

    // This is a dumb testing method to avoid using JSON for now. In the future I'll stop being stubborn and just use JSON but for now I don't want to.
    inline std::vector<DropTableSpec> loadTables(std::string_view f)
    {
        detail::ItemFile file(f);
        std::string line;

        std::vector<DropTableSpec> tables;
        DropTableSpec dropTable;
        std::optional<TierSpec> currentTier; // empty when we're not inside a tier
        bool inCharacter = false;

        while (file.readLine(line)) // the only place a line is ever read
        {
            std::istringstream iss(line); // fresh stream per line: no stale flags
            std::string keyword;
            if (!(iss >> keyword))
                continue; // blank line

            if (keyword == "Character")
            {
                inCharacter = true;
                dropTable = DropTableSpec{};
            }
            else if (keyword == "Tier")
            {
                std::string name, chance;
                if (!(iss >> name >> chance))
                {
                    std::cerr << "Bad tier line: " << line << std::endl;
                    continue;
                }
                currentTier.emplace(name, std::stod(chance));
            }
            else if (keyword == "Item")
            {
                if (!currentTier)
                {
                    std::cerr << "Item outside a tier: " << line << std::endl;
                    continue;
                }

                std::string_view rest = line;

                // Trim trailing whitespace (including '\r' from Windows line endings), then the ';'
                rest = rest.substr(0, rest.find_last_not_of(" \t\r") + 1);
                if (!rest.empty() && rest.back() == ';')

                    rest.remove_suffix(1);

                // The chance is the last word; everything before it is the item list
                const size_t lastSpace = rest.find_last_of(" \t");
                if (lastSpace == std::string_view::npos)
                {
                    std::cerr << "Bad item line: " << line << std::endl;
                    continue;
                }
                const std::string_view chance = rest.substr(lastSpace + 1);
                std::string_view itemList = rest.substr(0, lastSpace);

                // Split the item list on commas: "Item Axe, Item Sword" -> "Item Axe" and " Item Sword"
                std::vector<Item> items;
                bool ok = true;
                while (true)
                {
                    const size_t comma = itemList.find(',');
                    std::istringstream pieceStream{std::string(itemList.substr(0, comma))};

                    std::string itemKeyword, itemName, extra;
                    if (!(pieceStream >> itemKeyword >> itemName) || itemKeyword != "Item" || (pieceStream >> extra))
                    {
                        ok = false; // each piece must be exactly "Item <name>"
                        break;
                    }
                    items.emplace_back(itemName);

                    if (comma == std::string_view::npos)
                        break; // that was the last piece
                    itemList.remove_prefix(comma + 1);
                }

                if (!ok || items.empty())
                {
                    std::cerr << "Bad item line: " << line << std::endl;
                    continue;
                }

                currentTier->packs.push_back(ItemPack(items, std::stod(std::string(chance))));
            }
            else if (keyword == "}")
            {
                if (currentTier) // closing a tier
                {
                    dropTable.tiers.push_back(std::move(*currentTier));
                    currentTier.reset();
                }
                else if (inCharacter) // closing a character
                {
                    inCharacter = false;
                    tables.push_back(std::move(dropTable));
                    // dropTable is complete here: store it, validate it, etc.
                    // TODO validation step? For now I'm doing things by hand, so I know that it's correct, but a correctness step wouldn't hurt either here or when we are building everything.
                }
            }
        }
        return tables;
    }

} // namespace rolldown
