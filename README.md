<div align="center">

# Rolldown

[![C++17][cpp_img]][cpp_url]
[![Header-only][header_only_img]][repo_url]
[![Dependencies][deps_img]][repo_url]
[![License][license_img]][license_url]
[![CI][ci_img]][ci_url]
[![Compiler Tests][compilers_img]][ci_url]

A header-only C++17 library for **weighted random selection**: loot drops, enemy
actions, procedural generation, or any time one option must be picked from
many by probability. **You set the probabilities, and Rolldown picks as many
as you need.**

GAME ENGINE INTEGRATION:
[Unreal Engine](#unreal-engine)
[Unity](#unity)
[Godot](#godot)

</div>

## Quick start

Rolldown needs a **C++17** compiler and nothing else. There are no
dependencies and no build step.

Copy the [`include/rolldown/`][include_url] folder into your project and
include one header:

```cpp
#include "rolldown/rolldown.hpp"
```

Or let CMake add it for you, from a copy of this repository:

```cmake
add_subdirectory(external/rolldown)
target_link_libraries(my_game PRIVATE rolldown::rolldown)
```

Or straight from GitHub:

```cmake
include(FetchContent)
FetchContent_Declare(rolldown
    GIT_REPOSITORY https://github.com/ketchikan/rolldown.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(rolldown)

target_link_libraries(my_game PRIVATE rolldown::rolldown)
```

Then describe a table, build it once, and roll it as often as you need:

```cpp
#include <iostream>
#include <random>
#include "rolldown/rolldown.hpp"

using namespace rolldown;

// 1. Describe the table: tiers, and packs of items inside each tier
DropTableSpec spec;
spec.tiers.emplace_back("Legendary", 3.5, std::vector<ItemPack>{
    ItemPack({Item(1, "Sword")}, 60),
    ItemPack({Item(2, "Axe")}, 40),
});
spec.tiers.emplace_back("Common", 96.5, std::vector<ItemPack>{
    ItemPack({Item(3, "Potion")}, 90),
    ItemPack({}, 10), // an empty pack: nothing drops
});

// 2. Validate and build it once, at load time
const BuildResult built = buildDropTable(spec);
if (!built)
{
    std::cerr << toString(built.error) << " (tier " << built.tierIndex << ")\n";
    return 1;
}

// 3. Roll as often as you need
std::mt19937_64 rng{std::random_device{}()};
const ItemPack &drop = roll(built.table, rng);
for (const Item &item : drop.items)
    std::cout << "Dropped: " << item.name << "\n";
```

> 💡 A complete, runnable version of this code is in
> [`examples/json_loader/main.cpp`][example_json_url].

## Features

- **Exact percentages.** Chances are stored as integers, not floating-point
  numbers, so a 2.1% drop is exactly 2.1%, and the "adds up to 100%" checks
  are exact.
- **Validated before use.** Mistakes in a table are caught at load time, with
  an error that names the tier at fault, not in the middle of a game.
- **Bring your own random number generator.** Pass in any 64-bit generator
  (`std::mt19937_64`, xoshiro, PCG, your engine's own). Seed it and every drop
  is replayable, which is useful for save files, replays, multiplayer, and tests.
- **No exceptions.** Errors come back as values. Rolldown works in projects
  built with exceptions turned off (`-fno-exceptions`, which Unreal uses by
  default).
- **No dependencies, no file format.** Rolldown takes plain C++ structs. Load
  them from JSON, a database, a level editor, or build them in code.
- **Fast.** One roll is a branchless binary search over the table:
  _O(log n)_, about 4–7 ns on a modern CPU for typical tables (see
  [benchmarks](#tests-and-benchmarks)).
- **Statistically tested.** The test suite rolls each test table 10 million
  times and checks every pack's drop rate against its configured chance.

## How probabilities work

A **table** holds **tiers**. Each tier holds **packs**. Each pack holds zero or
more **items**, which all drop together. One roll picks exactly one pack.

The chance of a pack is its tier's chance multiplied by its own chance inside
that tier. For the table above:

| Pack        | Tier chance | Pack chance | Overall chance |
| ----------- | ----------- | ----------- | -------------- |
| Sword       | 3.5%        | 60%         | **2.1%**       |
| Axe         | 3.5%        | 40%         | **1.4%**       |
| Potion      | 96.5%       | 90%         | **86.85%**     |
| _(nothing)_ | 96.5%       | 10%         | **9.65%**      |

`buildDropTable` checks these rules and tells you which one a table breaks:

| Rule                                                      | Error if broken |
| --------------------------------------------------------- | --------------- |
| The table has at least one tier.                          | `NoTiers`       |
| Each tier has at least one pack.                          | `EmptyTier`     |
| No tier or pack chance is outside the 0-100 range.        | `OutOfRange`    |
| No tier or pack chance has 3+ decimal places of precision | `TooPrecise`    |
| The packs in each tier add up to exactly 100%.            | `ItemSumNot100` |
| The tiers add up to exactly 100%.                         | `TierSumNot100` |

A chance of `0` is allowed (that pack never drops), and so is a pack with no
items (a "nothing drops" result).

> ❗️ Chances have **two decimal places** of precision (`3.5` and `33.33` are
> fine). Extra decimals are rounded, so three packs of `33.333` add up to
> `99.99` and the tier is rejected. Use `33.33`, `33.33`, `33.34` instead.

## API

### Functions

| Function                                   | Returns               | Description                                                                                                                                                                         | When to call                     |
| ------------------------------------------ | --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------- |
| `buildDropTable(spec)`                     | `BuildResult`         | Checks a `DropTableSpec` against the rules above and builds a `DropTable`.                                                                                                          | Once, at load                    |
| `rollIndexes(table, rng, count, iterator)` | `iterator`            | Picks a selected number of indexes into `table.packs()`. Useful for selecting many random actions at once. Returns an iterator like a vector or array (matches the type passed in). | Once for many selections at once |
| `rollIndexes(table, rng, count)`           | `std::vector<size_t>` | Picks a selected number of indexes into `table.packs()`. Useful for selecting many random actions at once. Returns a vector of `size_t` indexes.                                    | Once for many selections at once |
| `roll(table, rng)`                         | `const ItemPack &`    | Picks one pack.                                                                                                                                                                     | Each drop                        |
| `rollIndex(table, rng)`                    | `size_t`              | Picks one pack and returns its index in `table.packs()`. Useful when the index _is_ the answer, for example an action number in machine learning.                                   | Each drop                        |
| `toString(error)`                          | `const char *`        | A readable message for a `BuildError`.                                                                                                                                              | On an error                      |

`rng` can be any generator with the standard C++ generator interface that
produces full 64-bit values. A 32-bit generator such as `std::mt19937` is
rejected at compile time with a clear message.

### Types

| Type            | Members                                        | Description                                                                |
| --------------- | ---------------------------------------------- | -------------------------------------------------------------------------- |
| `Item`          | `id`, `name`                                   | One item. `id` is _your_ identifier for it.                                |
| `ItemPack`      | `items`, `probability`                         | Items that drop together, and the pack's chance inside its tier.           |
| `TierSpec`      | `name`, `probability`, `packs`                 | A group of packs, and the tier's chance inside the table.                  |
| `DropTableSpec` | `id`, `tiers`                                  | A full table description. `id` is your key for it (for example, an enemy). |
| `DropTable`     | `packs()`, `size()`                            | A validated table, ready to roll. Only `buildDropTable` can make one.      |
| `BuildResult`   | `table`, `error`, `tierIndex`, `operator bool` | The built table, or the first error and the tier that caused it.           |

Packs keep the order they have in the spec: all packs of the first tier, then
all packs of the second tier, and so on. `rollIndex` uses this order.

## Loading your data

Rolldown has no file format of its own. Fill in the `Spec` structs from
whatever your game already uses.

We built an example using [nlohmann/json][nlohmann_url]. Check
[`examples/json_loader/`][example_json_url] folder for a complete, tested loader
that you can copy into your project and change to fit your data. It:

- reads from a file or from JSON already in memory,
- never throws, even on invalid JSON,
- reports problems with their exact location, for example
  `A required field is missing. at /characters/0/tiers/1/probability`,
- accepts comments in the JSON file.

```cpp
#include "json_loader.hpp"

const auto loaded = rolldown_json::loadTablesFromJson("loot.json");
if (!loaded)
    std::cerr << rolldown_json::toString(loaded.error) << " at " << loaded.where << "\n";

std::unordered_map<uint64_t, rolldown::DropTable> tables;
for (const rolldown::DropTableSpec &spec : loaded.tables)
{
    rolldown::BuildResult built = rolldown::buildDropTable(spec);
    if (built)
        tables[spec.id] = std::move(built.table);
}
```

The JSON shape it reads:

```json
{
  "characters": [
    {
      "id": 1,
      "tiers": [
        {
          "name": "Legendary",
          "probability": 3.5,
          "packs": [
            { "probability": 60, "items": [{ "id": 1, "name": "Sword" }] },
            { "probability": 40, "items": [{ "id": 2, "name": "Axe" }] }
          ]
        }
      ]
    }
  ]
}
```

## Hooking it into your project

- **Build once, roll many times.** Call `buildDropTable` when the game loads
  its data, and keep the `DropTable`. Do not build a table on each roll.
- **Check the result.** A `DropTable` from a failed build is empty. Rolling an
  empty table stops with an `assert` in debug builds, and is undefined
  behavior in release builds.
- **Use `Item::id` as the link to your game.** Rolldown does not know what a
  "Sword" is. Map the `id` to your own item data, prefab, or database row.
- **Own your random number generator.** Keep one generator for each system
  that needs its own sequence of drops. With a fixed seed, the same actions
  give the same drops every time.
- **Threads.** Rolling only reads a `DropTable`, so many threads can share one
  table. A generator is not safe to share: give each thread its own.

### Unreal Engine

Support for Unreal Engine is planned and in progress.

### Unity

Support for Unity is planned and in progress.

### Godot

Support for Godot is planned and in progress.

## How it works

1. **Integers, not floats.** Each chance is scaled by 100 (`3.5%` becomes
   `350`). A pack's share of the roll is _tier × pack_, out of
   10,000 × 10,000 = **100,000,000**. All sums and checks are exact integer
   operations.
2. **Cutoffs at build time.** `buildDropTable` stores the running total of the
   shares: one increasing "cutoff" value for each pack.
3. **An unbiased random number.** Each roll gets a uniform number from
   0 to 99,999,999 with
   [Lemire's nearly divisionless method][lemire_url]. A plain `rng() % n` is
   slightly biased towards smaller numbers. This method is not, and it is faster.
4. **Binary search.** Uses a branchless binary search to find the first cutoff above the
   random number. That cutoff's pack is the result: _O(log n)_.
5. **Cache-friendly layout.** The cutoffs are stored in their own array,
   separate from the packs (struct-of-arrays). The search reads only cutoffs,
   and sixteen of them fit in one 64-byte cache line.
6. **Protected once validated.** `DropTable` keeps its data private. Only
   `buildDropTable` can fill it, so a table that passed validation cannot be
   changed into an invalid one.

## Tests and benchmarks

The tests and benchmarks use [Catch2][catch2_url], which CMake downloads
automatically. The examples use the copies of nlohmann/json and
[Xoshiro-cpp][xoshiro_url] in `third_party/`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # run the tests
./build/rolldown_bench                       # run the benchmarks
./build/rolldown_example_json                # run the JSON example
```

| Option                    | Description                         | Default                                                |
| ------------------------- | ----------------------------------- | ------------------------------------------------------ |
| `ROLLDOWN_BUILD_TESTS`    | Build the tests and the benchmarks. | `ON` when built directly, `OFF` inside another project |
| `ROLLDOWN_BUILD_EXAMPLES` | Build the examples.                 | `ON` when built directly, `OFF` inside another project |

**What the tests check:**

- **Each validation rule**, with the error and the tier it reports.
- **The JSON example loader**: valid files, invalid JSON, missing fields, and
  wrong types, with the reported location of each problem.
- **Drop rates.** Each test table is rolled 10 million times with both
  `std::mt19937_64` and xoshiro256++. Every pack's count must be within
  5 standard deviations of its expected count. A `0%` pack must never drop. The
  generators use a fixed seed, so the result is the same on every run.

**Benchmarks** (Apple M2 Pro, Apple Clang 16, Release build, measured with
Catch2):

| Time for each roll | `std::mt19937_64` | xoshiro256++ |
| ------------------ | ----------------- | ------------ |
| Generator only     | 4.5 ns            | 2.8 ns       |
| Full `rollIndex`   | 6.7 ns            | 3.8 ns       |

Most of each roll is the random number generator itself. A faster generator
makes the full roll faster, with no change to Rolldown.

| Packs in the table                       | 2      | 8      | 32     | 128    | 1000    |
| ---------------------------------------- | ------ | ------ | ------ | ------ | ------- |
| `rollIndex`, branchless search (current) | 3.3 ns | 3.9 ns | 4.7 ns | 6.6 ns | 9.3 ns  |
| `rollIndex`, `std::upper_bound` (v0.1)   | 3.4 ns | 6.8 ns | 6.4 ns | 7.6 ns | 12.0 ns |

The search compares with a conditional select instead of a branch. A branch on
a random value is mispredicted about half of the time, and that cost was larger
than the rest of the roll for small tables.

| 1000 rolls, 100 packs, xoshiro256++                  | Total  | For each roll |
| ---------------------------------------------------- | ------ | ------------- |
| `rollIndex` in a loop                                | 5.7 µs | 5.7 ns        |
| `rollIndexes` into a plain array                     | 6.1 µs | 6.1 ns        |
| `rollIndexes`, returns a new vector                  | 6.1 µs | 6.1 ns        |
| `rollIndexes` into a reused vector (`back_inserter`) | 6.4 µs | 6.4 ns        |

All four are the same within measurement noise (±0.2–0.7 µs). `rollIndexes`
is not faster than a loop. Its purpose is control of memory: write into a
buffer that you reuse every frame, and a batch of rolls needs no allocation.

## Design decisions

- **Errors are values, not exceptions,** so that Rolldown works in game
  engines that turn exceptions off. `BuildResult` works like C++23's
  `std::expected`, but needs only C++17.
- **The caller owns the random number generator,** so that drops can be
  seeded, replayed, and tested, and so that a faster generator needs no
  library change.
- **`DropTable` is a class, and the `Spec` types are structs.** An invalid
  spec is allowed to exist, because the builder reports it. An invalid built
  table must never exist, so its data is private.
- **The JSON loader is an example, not part of the library.** A file format is
  a decision for each game. The library takes plain structs, and the example
  shows one way to fill them.
- **Designed for C++17** Many game projects still use C++17, and a header-only
  library can only be used by projects with the same standard or newer.

## Roadmap

- **Unreal Engine Support:** Add code examples and support for Unreal Engine integration.
- **Unity Support:** Add code examples and support for Unity integration.
- **Godot Support:** Add code examples and support for Godot integration.
- **Pity system:** guarantee a drop after a set number of rolls without it.
  Optional, for each item.
- **Tierless tables:** set each pack's overall chance directly, with no tiers.
- **Roll counts:** let a table decide how many rolls to make, with a chance for
  each count.
- **Alias method:** _O(1)_ rolls for very large tables.

## License

Rolldown is free and open-source software under the [MIT License][license_url].

The tests, benchmarks, and examples use [Catch2][catch2_url] (BSL-1.0),
[nlohmann/json][nlohmann_url] (MIT), and [Xoshiro-cpp][xoshiro_url] (MIT). The
library itself does not use them.

<!-- Badges -->

[cpp_img]: https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus
[cpp_url]: https://en.cppreference.com/w/cpp/17
[header_only_img]: https://img.shields.io/badge/header--only-yes-success?style=for-the-badge&logo=none
[deps_img]: https://img.shields.io/badge/dependencies-none-success?style=for-the-badge&logo=none
[license_img]: https://img.shields.io/badge/license-MIT-blue?style=for-the-badge&logo=none
[ci_img]: https://img.shields.io/github/actions/workflow/status/ketchikan/rolldown/ci.yml?branch=main&style=for-the-badge&label=CI
[ci_url]: https://github.com/ketchikan/rolldown/actions/workflows/ci.yml
[compilers_img]: https://img.shields.io/badge/compilers-GCC%20%7C%20Clang%20%7C%20MSVC-informational?style=for-the-badge&logo=none

<!-- Repository -->

[repo_url]: https://github.com/ketchikan/rolldown
[license_url]: https://github.com/ketchikan/rolldown/blob/main/LICENSE
[include_url]: https://github.com/ketchikan/rolldown/tree/main/include/rolldown
[example_json_url]: https://github.com/ketchikan/rolldown/tree/main/examples/json_loader
[lessons_url]: https://github.com/ketchikan/rolldown/blob/main/docs/LESSONS.md

<!-- Other projects -->

[catch2_url]: https://github.com/catchorg/Catch2
[nlohmann_url]: https://github.com/nlohmann/json
[xoshiro_url]: https://github.com/Reputeless/Xoshiro-cpp
[lemire_url]: https://arxiv.org/abs/1805.10941
