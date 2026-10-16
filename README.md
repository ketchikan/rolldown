# Rolldown

Header-only C++20 drop tables for games: tiered loot with exact percentages,
no floating-point drift, and fast O(n) rolls (pity system planned).

​```cpp
#include "rolldown/rolldown.hpp"

auto table = rolldown::loadTables("enemies.txt")[0];
auto cutoffs = rolldown::buildCutoffs(table); // do once at load time
size_t idx = rolldown::roll(cutoffs); // do per drop
​```

Drop the folder into your project and include it. No build step and no dependencies.

## Features

What can it do? What makes it different from the obvious approach?

- Rather than utilizing C++23's `std::expected`, created custom error reporting usable in compilers as early as C++17.

## Getting Started

Download the repository. To run it with the benchmark:

```CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # configure: generate the build system into build/
cmake --build build                              # build
./build/rolldown_bench                           # run from the repo root (tests/*.txt paths are relative)

```

Optional flags:
`-DROLLDOWN_BUILD_BENCH=OFF` to turn off building the benchmark

If you copied the folder into your repo, add the following to your CMakeLists.txt file:

```CMake
add_subdirectory(external/rolldown)
target_link_libraries(my_game PRIVATE rolldown::rolldown)
```

If you prefer having CMake download this repository from GitHub to include in your project, add this to your CMakeLists.txt file isntead:

```CMake
include(FetchContent)
FetchContent_Declare(rolldown
    GIT_REPOSITORY https://github.com/ketchikan/rolldown.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(rolldown)

target_link_libraries(my_game PRIVATE rolldown::rolldown)
```

### Requirements

What does someone need installed? (compiler, C++ standard, libraries)

### Building

What exact commands build it?

### Running

What command runs it, and what should they expect to see?

## Usage

Show, don't tell: a small, complete example from input to output.

## Config File Format

A reference for every keyword: what it means, what values it takes,
and the rules it must follow.

## How It Works

The design, explained for someone curious about the internals.

## Roadmap

What's planned, or not done yet?
