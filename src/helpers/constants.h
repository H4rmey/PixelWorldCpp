#pragma once

#include <cstddef>

static constexpr int CHUNK_COUNT_X = 16;
static constexpr int CHUNK_COUNT_Y = 9;
static constexpr int CHUNK_SIZE_X = 16;
static constexpr int CHUNK_SIZE_Y = 16;

static constexpr std::size_t CHUNK_COUNT = CHUNK_COUNT_X * CHUNK_COUNT_Y;
static constexpr std::size_t WORLD_SIZE_X = CHUNK_COUNT_X * CHUNK_COUNT_X;
static constexpr std::size_t WORLD_SIZE_Y = CHUNK_COUNT_Y * CHUNK_COUNT_Y;
