#pragma once

#include <cstddef>

#define tiny

static constexpr int WINDOW_SIZE_X = 1280 / 2;
static constexpr int WINDOW_SIZE_Y = 768 / 2;

static constexpr int CHUNK_SIZE_X = 32;
static constexpr int CHUNK_SIZE_Y = 32;

static constexpr int MAX_FRAMERATE = 60;
static constexpr int GRAVITY       = 32;
static constexpr int CELL_SIZE     = 2;
static constexpr int BRUSH_SIZE    = 1;

static constexpr std::size_t WORLD_SIZE_X = WINDOW_SIZE_X / CELL_SIZE;
static constexpr std::size_t WORLD_SIZE_Y = WINDOW_SIZE_Y / CELL_SIZE;

static constexpr int CHUNK_COUNT_X = WORLD_SIZE_X / CHUNK_SIZE_Y;
static constexpr int CHUNK_COUNT_Y = WORLD_SIZE_Y / CHUNK_SIZE_Y;

static constexpr std::size_t CHUNK_COUNT = CHUNK_COUNT_Y * CHUNK_COUNT_X;
