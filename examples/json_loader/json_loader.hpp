#pragma once

// Optional JSON loader. Requires nlohmann/json 3.x on the include path.
// Not included by rolldown.hpp: include this header only if your project uses nlohmann/json.

#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "rolldown/types.hpp"

namespace rolldown_json
{
    using rolldown::DropTableSpec;
    using rolldown::Item;
    using rolldown::ItemPack;
    using rolldown::TierSpec;
    /**
    @enum LoadError

    @brief Problems with the file or with the shape of the JSON.

    @note The loader checks shape and types only. buildDropTable checks the probabilities.
    */
    enum class LoadError : uint8_t
    {
        None,
        FileNotFound,
        InvalidJson,  // The text is not valid JSON
        MissingField, // A required field is not there
        WrongType,    // A field is there, but has the wrong type (for example, "probability": "3.5")
    };

    /**
    @struct LoadResult

    @brief The loaded table specs, or the first problem found.

    @note Upon failure, `tables` is empty.
    */
    struct LoadResult
    {
        std::vector<DropTableSpec> tables;
        LoadError error = LoadError::None;
        std::string where; // JSON path of the problem, for example "/characters/0/tiers/1/probability"

        explicit operator bool() const { return error == LoadError::None; }
    };

    /**
    @fn toString

    @brief Convert a `LoadError` value to a string to output error messages.
    */
    inline const char *toString(LoadError e)
    {
        switch (e)
        {
        case LoadError::None:
            return "No Error";
        case LoadError::FileNotFound:
            return "The file could not be opened.";
        case LoadError::InvalidJson:
            return "The file is not valid JSON.";
        case LoadError::MissingField:
            return "A required field is missing.";
        case LoadError::WrongType:
            return "A field has the wrong type.";
        }
        return "Unknown error.";
    }

    namespace detail
    {
        enum class JsonType : uint8_t
        {
            Object,
            Array,
            Number,
            String,
            UnsignedInt,
        };

        inline bool hasType(const nlohmann::json &value, JsonType type)
        {
            switch (type)
            {
            case JsonType::Object:
                return value.is_object();
            case JsonType::Array:
                return value.is_array();
            case JsonType::Number:
                return value.is_number();
            case JsonType::String:
                return value.is_string();
            case JsonType::UnsignedInt:
                return value.is_number_unsigned();
            }
            return false;
        }

        inline bool fail(LoadResult &r, LoadError error, std::string where)
        {
            r.error = error;
            r.where = std::move(where);
            return false;
        }

        /**
        @fn getField

        @brief Find `key` in `obj` and check its type. Nothing here can throw.

        @return A pointer to the value, or nullptr after it records the problem in `r`.
        */
        inline const nlohmann::json *getField(const nlohmann::json &obj, const char *key, JsonType type,
                                              const std::string &path, LoadResult &r)
        {
            if (!obj.is_object())
            {
                fail(r, LoadError::WrongType, path.empty() ? "/" : path);
                return nullptr;
            }

            const auto it = obj.find(key);
            if (it == obj.end())
            {
                fail(r, LoadError::MissingField, path + "/" + key);
                return nullptr;
            }
            if (!hasType(*it, type))
            {
                fail(r, LoadError::WrongType, path + "/" + key);
                return nullptr;
            }
            return &*it;
        }

        // One function for each level of the JSON. Each one returns false on the first problem.

        inline bool parseItem(const nlohmann::json &j, const std::string &path, LoadResult &r, std::vector<Item> &out)
        {
            const auto *id = getField(j, "id", JsonType::UnsignedInt, path, r);
            if (!id)
                return false;
            // const auto *name = getField(j, "name", JsonType::String, path, r);
            // if (!name)
            //     return false;

            out.emplace_back(id->get<uint64_t>());
            return true;
        }

        inline bool parsePack(const nlohmann::json &j, const std::string &path, LoadResult &r, std::vector<ItemPack> &out)
        {
            const auto *probability = getField(j, "probability", JsonType::Number, path, r);
            if (!probability)
                return false;
            const auto *items = getField(j, "items", JsonType::Array, path, r);
            if (!items)
                return false;

            // An empty "items" array is valid: it is a "nothing drops" pack.
            std::vector<Item> parsed;
            for (size_t i = 0; i < items->size(); ++i)
            {
                if (!parseItem((*items)[i], path + "/items/" + std::to_string(i), r, parsed))
                    return false;
            }

            out.emplace_back(parsed, probability->get<double>());
            return true;
        }

        inline bool parseTier(const nlohmann::json &j, const std::string &path, LoadResult &r, std::vector<TierSpec> &out)
        {
            const auto *name = getField(j, "name", JsonType::String, path, r);
            if (!name)
                return false;
            const auto *probability = getField(j, "probability", JsonType::Number, path, r);
            if (!probability)
                return false;
            const auto *packs = getField(j, "packs", JsonType::Array, path, r);
            if (!packs)
                return false;

            std::vector<ItemPack> parsed;
            for (size_t p = 0; p < packs->size(); ++p)
            {
                if (!parsePack((*packs)[p], path + "/packs/" + std::to_string(p), r, parsed))
                    return false;
            }

            out.emplace_back(name->get<std::string>(), probability->get<double>(), parsed);
            return true;
        }

        inline bool parseTable(const nlohmann::json &j, const std::string &path, LoadResult &r, std::vector<DropTableSpec> &out)
        {
            const auto *id = getField(j, "id", JsonType::UnsignedInt, path, r);
            if (!id)
                return false;
            const auto *tiers = getField(j, "tiers", JsonType::Array, path, r);
            if (!tiers)
                return false;

            DropTableSpec spec;
            spec.id = id->get<uint64_t>();
            for (size_t t = 0; t < tiers->size(); ++t)
            {
                if (!parseTier((*tiers)[t], path + "/tiers/" + std::to_string(t), r, spec.tiers))
                    return false;
            }

            out.push_back(std::move(spec));
            return true;
        }
    }

    /**
    @fn parseTables

    @brief Convert JSON that is already in memory to table specs. Use this if your game already parsed the JSON,
           or keeps the tables inside a larger JSON file.

    @param[in] root A JSON object with a "characters" array.

    @return `LoadResult` the specs, or the first problem and its JSON path.

    @code{.cpp}
    const auto loaded = rolldown::parseTables(gameData["lootTables"]);
    if (!loaded)
        std::cerr << rolldown::toString(loaded.error) << " at " << loaded.where << "\n";
    @endcode
    */
    inline LoadResult parseTables(const nlohmann::json &root)
    {
        LoadResult r;
        const auto *characters = detail::getField(root, "characters", detail::JsonType::Array, "", r);
        if (!characters)
            return r;

        for (size_t c = 0; c < characters->size(); ++c)
        {
            if (!detail::parseTable((*characters)[c], "/characters/" + std::to_string(c), r, r.tables))
            {
                r.tables.clear();
                return r;
            }
        }
        return r;
    }

    /**
    @fn loadTablesFromJson

    @brief Read a JSON file and convert it to table specs. Comments (line and block) in the file are permitted.

    @return `LoadResult` the specs, or the first problem. For file problems, `where` holds the file name.
    */
    inline LoadResult loadTablesFromJson(std::string_view fileName)
    {
        LoadResult r;

        std::ifstream stream{std::string(fileName)};
        if (!stream)
        {
            detail::fail(r, LoadError::FileNotFound, std::string(fileName));
            return r;
        }

        // allow_exceptions = false: invalid JSON gives a "discarded" value, not an exception
        const nlohmann::json root = nlohmann::json::parse(stream, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
        if (root.is_discarded())
        {
            detail::fail(r, LoadError::InvalidJson, std::string(fileName));
            return r;
        }

        return parseTables(root);
    }

} // namespace rolldown_json
