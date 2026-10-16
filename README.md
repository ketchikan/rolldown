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

## Getting Started

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
