#pragma once

#include <cstddef>

#define medium

static constexpr int CHUNK_SIZE_X = 16*2;
static constexpr int CHUNK_SIZE_Y = 16*2;

#ifdef test
	static constexpr int CHUNK_COUNT_X = 4;
	static constexpr int CHUNK_COUNT_Y = 2;
	static constexpr int CELL_SIZE = 5;
	static constexpr int BRUSH_SIZE = 10;
#endif

#ifdef large
	static constexpr int CHUNK_COUNT_X = 32;
	static constexpr int CHUNK_COUNT_Y = 16;
	static constexpr int CELL_SIZE = 3;
	static constexpr int BRUSH_SIZE = 10;
#endif

#ifdef medium
	static constexpr int CHUNK_COUNT_X = 16;
	static constexpr int CHUNK_COUNT_Y = 8;
	static constexpr int CELL_SIZE = 1;
	static constexpr int BRUSH_SIZE = 8;
#endif

#ifdef small
	static constexpr int CHUNK_COUNT_X = 8;
	static constexpr int CHUNK_COUNT_Y = 4;
	static constexpr int CELL_SIZE = 7;
	static constexpr int BRUSH_SIZE = 5;
#endif

#ifdef tiny
	static constexpr int CHUNK_COUNT_X = 4;
	static constexpr int CHUNK_COUNT_Y = 2;
	static constexpr int CELL_SIZE = 9;
	static constexpr int BRUSH_SIZE = 5;
#endif

static constexpr std::size_t CHUNK_COUNT = CHUNK_COUNT_X * CHUNK_COUNT_Y;
static constexpr std::size_t WORLD_SIZE_X = CHUNK_COUNT_X * CHUNK_COUNT_X;
static constexpr std::size_t WORLD_SIZE_Y = CHUNK_COUNT_Y * CHUNK_COUNT_Y;
