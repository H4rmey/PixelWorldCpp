#pragma once

#include <cstddef>

static constexpr int CHUNK_COUNT_X = 5;
static constexpr int CHUNK_COUNT_Y = 6;
static constexpr int CHUNK_SIZE_X = 16;
static constexpr int CHUNK_SIZE_Y = 9;

static constexpr std::size_t CHUNK_COUNT = CHUNK_COUNT_X * CHUNK_COUNT_Y;
