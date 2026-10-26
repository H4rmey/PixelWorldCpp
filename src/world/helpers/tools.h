#pragma once

#include "./constants.h"
#include <cstddef>
#include <random>

inline bool fastRandomBool(float distribution)
{
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(distribution);
    return dist(rng);
}

inline bool fastRandomBool()
{
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(0.5);
    return dist(rng);
}

inline int fastRandomIntRange(int min = 0, int max = 255)
{
    static thread_local std::mt19937 rng(std::random_device{}());
    // Note: distribution range must be set at construction or passed as param
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloatRange(float min = 0.0f, float max = 1.0f)
{
    static thread_local std::mt19937      rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloat01()
{
    static thread_local std::mt19937                          rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng);
}

static inline int floor_div(int a, int b)
{ // b > 0
    int q = a / b;
    int r = a % b;
    if (r && a < 0)
    {
        --q;
    }
    return q;
}

static inline int floor_mod(int a, int b)
{ // b > 0
    int m = a % b;
    if (m < 0)
    {
        m += b;
    }
    return m;
}

// Simple struct to hold the result
struct Coord
{
    int x;
    int y;
};

// Converts a 1D world index into 2D x and y coordinates
inline Coord index_to_world_coords(std::size_t index)
{
    return {static_cast<int>(index % WORLD_SIZE_X), static_cast<int>(index / WORLD_SIZE_X)};
}
