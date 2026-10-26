#pragma once

#include "./FastNoiseLite.h"        // Ensure this is in your include path
#include "./helpers/element_type.h" // Ensure this is in your include path
#include "world/helpers/constants.h"
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <vector>

class WorldGenerator
{
  public:
    std::vector<uint8_t> generatedWorld;

    struct Config
    {
        int   width           = WORLD_SIZE_X;
        int   height          = WORLD_SIZE_Y;
        int   seed            = 123456789;
        float surfaceRatio    = 0.4f;
        float blockSize       = 4.0f;
        int   caIterations    = 2;
        bool  enablePitfalls  = true;
        bool  enableEntrances = true;
        float crustThickness  = 35.0f;
    };

    WorldGenerator() {}

    void Generate(const Config &config)
    {
        m_width  = config.width;
        m_height = config.height;
        generatedWorld.assign(
            m_width * m_height,
            (uint8_t)ElementType::Empty); // Flat 1D array for better cache locality

        FastNoiseLite noise;
        noise.SetSeed(config.seed);
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

        float surfaceBaseY = config.height * config.surfaceRatio;

        // 1. Generate Terrain
        for (int y = 0; y < config.height; y++)
        {
            for (int x = 0; x < config.width; x++)
            {
                float fx = static_cast<float>(x);
                float fy = static_cast<float>(y);

                float surfaceY = surfaceBaseY +
                                 (noise.GetNoise(fx * 0.8f, 0.0f) *
                                  120.0f) + // Note: FastNoiseLite takes inputs usually unscaled, so
                                            // we multiply by 1000 for standard scale matching
                                 (noise.GetNoise(fx * 5.0f, 0.0f) * 10.0f);

                if (fy < surfaceY)
                {
                    SetBlock(x, y, (uint8_t)ElementType::Empty);
                    continue;
                }

                float depth = fy - surfaceY;

                float qx = std::floor(fx / config.blockSize) * config.blockSize;
                float qy = std::floor(fy / config.blockSize) * config.blockSize;

                // Tunnels
                float tunnelNoise     = getFBm(noise, qx * 15.0f, qy * 40.0f, 2);
                float tunnelThickness = 0.04f;
                bool  isAir           = std::abs(tunnelNoise) < tunnelThickness;

                // Pitfalls
                if (config.enablePitfalls)
                {
                    float pitfallNoise = getFBm(noise, qx * 36.0f, qy * 5.0f, 2);
                    if (std::abs(pitfallNoise) < 0.015f)
                    {
                        isAir = true;
                    }
                }

                // Caverns
                float cavernNoise = getFBm(noise, qx * 20.0f, qy * 20.0f, 2);
                if (cavernNoise > 0.75f)
                {
                    isAir = true;
                }

                // Crust Layer Check
                float crustNoiseValue = (noise.GetNoise(fx * 1.0f, 100000.0f) + 1.0f) / 2.0f;
                float crustCurrentThickness =
                    (config.crustThickness - 15.0f) + (crustNoiseValue * 30.0f);

                if (depth < crustCurrentThickness)
                {
                    if (config.enableEntrances)
                    {
                        float entranceNoise = noise.GetNoise(qx * 3.0f, qy * 20.0f);
                        isAir               = (std::abs(entranceNoise) < 0.03f);
                    }
                    else
                    {
                        isAir = false;
                    }
                }

                SetBlock(x, y, isAir ? (uint8_t)ElementType::Empty : (uint8_t)ElementType::Sand);
            }
        }

        // 2. Apply Cellular Automata Smoothing
        if (config.caIterations > 0)
        {
            applyCellularAutomata(config);
        }
    }

    uint8_t GetBlock(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
        {
            return (uint8_t)ElementType::Sand;
        }
        return generatedWorld[y * m_width + x];
    }

    const std::vector<uint8_t> &GetWorldData() const { return generatedWorld; }

  private:
    int m_width  = 0;
    int m_height = 0;

    void SetBlock(int x, int y, uint8_t material) { generatedWorld[y * m_width + x] = material; }

    // Helper to match the manual FBm loop from the Go code using FastNoiseLite's base noise.
    // We scale the frequency inputs up because FastNoiseLite expects larger coordinate values than
    // the custom Go Perlin.
    float getFBm(FastNoiseLite &noise, float x, float y, int octaves)
    {
        float total = 0.0f;
        float freq  = 1.0f;
        float amp   = 1.0f;
        float max   = 0.0f;

        for (int i = 0; i < octaves; i++)
        {
            total += noise.GetNoise(x * freq, y * freq) * amp;
            max += amp;
            amp *= 0.5f;
            freq *= 2.0f;
        }
        return total / max;
    }

    void applyCellularAutomata(const Config &config)
    {
        int                  bSize  = std::max(1, static_cast<int>(config.blockSize));
        std::vector<uint8_t> buffer = generatedWorld;

        for (int i = 0; i < config.caIterations; i++)
        {
            for (int y = 0; y < m_height; y += bSize)
            {
                for (int x = 0; x < m_width; x += bSize)
                {

                    // Skip sky to save performance
                    if (GetBlock(x, y) == (uint8_t)ElementType::Empty &&
                        y < static_cast<int>(m_height * 0.4f) - 150)
                    {
                        continue;
                    }

                    int wallCount = 0;

                    // Check 3x3 neighbors
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            if (dy == 0 && dx == 0)
                            {
                                continue;
                            }

                            int nx = x + (dx * bSize);
                            int ny = y + (dy * bSize);

                            if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height)
                            {
                                wallCount++;
                            }
                            else if (GetBlock(nx, ny) == (uint8_t)ElementType::Sand)
                            {
                                wallCount++;
                            }
                        }
                    }

                    uint8_t currentState = GetBlock(x, y);
                    uint8_t newState     = currentState;

                    if (currentState == (uint8_t)ElementType::Sand)
                    {
                        if (wallCount < 4)
                        {
                            newState = (uint8_t)ElementType::Empty;
                        }
                    }
                    else
                    {
                        if (wallCount >= 5)
                        {
                            newState = (uint8_t)ElementType::Sand;
                        }
                    }

                    // Apply the new state in chunks to preserve the blocky aesthetic
                    for (int by = 0; by < bSize; by++)
                    {
                        for (int bx = 0; bx < bSize; bx++)
                        {
                            if (y + by < m_height && x + bx < m_width)
                            {
                                buffer[(y + by) * m_width + (x + bx)] = newState;
                            }
                        }
                    }
                }
            }
            generatedWorld = buffer;
        }
    }
};
