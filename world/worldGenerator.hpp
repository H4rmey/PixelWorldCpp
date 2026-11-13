#pragma once

#include "FastNoiseLite.h"
#include "helpers/constants.h"
#include "helpers/element_type.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
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
        float surfaceRatio    = 0.2f;
        float blockSize       = 2.0f;
        int   caIterations    = 1;
        bool  enablePitfalls  = true;
        bool  enableEntrances = true;
        float crustThickness  = 35.0f;
    };

    WorldGenerator() = default;
    bool ExportImage(const char *filename) const
    {
        std::ofstream file(filename, std::ios::binary);

        if (!file)
        {
            return false;
        }

        // PPM header
        file << "P6\n" << m_width << " " << m_height << "\n255\n";

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                const uint8_t block = GetBlock(x, y);

                unsigned char rgb[3];

                if (block == EmptyMaterial())
                {
                    // Air - dark blue/black
                    rgb[0] = 20;
                    rgb[1] = 25;
                    rgb[2] = 35;
                }
                else
                {
                    // Surface gets green, underground is brown/grey
                    bool surface = y > 0 && GetBlock(x, y - 1) == EmptyMaterial();

                    if (surface)
                    {
                        rgb[0] = 50;
                        rgb[1] = 160;
                        rgb[2] = 70;
                    }
                    else
                    {
                        rgb[0] = 90;
                        rgb[1] = 80;
                        rgb[2] = 80;
                    }
                }

                file.write(reinterpret_cast<const char *>(rgb), sizeof(rgb));
            }
        }

        return true;
    }
    void Generate(const Config &config)
    {
        m_width  = config.width;
        m_height = config.height;

        generatedWorld.assign(static_cast<size_t>(m_width) * static_cast<size_t>(m_height),
                              EmptyMaterial());

        FastNoiseLite noise;
        noise.SetSeed(config.seed);
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

        // All coordinate scaling is performed explicitly below.
        noise.SetFrequency(1.0f);

        const float surfaceBaseY = static_cast<float>(m_height) * config.surfaceRatio;

        // Generate the initial terrain.
        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                const float fx = static_cast<float>(x);
                const float fy = static_cast<float>(y);

                const float surfaceY = surfaceBaseY + noise.GetNoise(fx * 0.0008f, 0.0f) * 120.0f +
                                       noise.GetNoise(fx * 0.005f, 0.0f) * 10.0f;

                // Everything above the surface is empty sky.
                if (fy < surfaceY)
                {
                    SetBlock(x, y, EmptyMaterial());
                    continue;
                }

                const float depth = fy - surfaceY;

                const float safeBlockSize = std::max(1.0f, config.blockSize);

                const float qx = std::floor(fx / safeBlockSize) * safeBlockSize;

                const float qy = std::floor(fy / safeBlockSize) * safeBlockSize;

                // Horizontal and irregular tunnels.
                const float tunnelNoise = GetFBm(noise, qx * 0.015f, qy * 0.04f, 2);

                constexpr float tunnelThickness = 0.04f;

                bool isAir = std::abs(tunnelNoise) < tunnelThickness;

                // Mostly vertical passages.
                if (config.enablePitfalls)
                {
                    const float pitfallNoise = GetFBm(noise, qx * 0.036f, qy * 0.005f, 2);

                    if (std::abs(pitfallNoise) < 0.015f)
                    {
                        isAir = true;
                    }
                }

                // Larger cavern areas.
                const float cavernNoise = GetFBm(noise, qx * 0.02f, qy * 0.02f, 2);

                if (cavernNoise > 0.75f)
                {
                    isAir = true;
                }

                // Variable solid crust directly below the surface.
                const float crustNoiseValue = (noise.GetNoise(fx * 0.001f, 100.0f) + 1.0f) / 2.0f;

                const float crustCurrentThickness =
                    (config.crustThickness - 15.0f) + crustNoiseValue * 30.0f;

                if (depth < crustCurrentThickness)
                {
                    if (config.enableEntrances)
                    {
                        const float entranceNoise = noise.GetNoise(qx * 0.003f, qy * 0.02f);

                        isAir = std::abs(entranceNoise) < 0.03f;
                    }
                    else
                    {
                        isAir = false;
                    }
                }

                SetBlock(x, y, isAir ? EmptyMaterial() : SolidMaterial());
            }
        }

        if (config.caIterations > 0)
        {
            ApplyCellularAutomata(config);
        }
    }

    uint8_t GetBlock(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
        {
            return SolidMaterial();
        }

        return generatedWorld[static_cast<size_t>(y) * static_cast<size_t>(m_width) +
                              static_cast<size_t>(x)];
    }

    const std::vector<uint8_t> &GetWorldData() const { return generatedWorld; }

  private:
    int m_width  = 0;
    int m_height = 0;

    static constexpr uint8_t EmptyMaterial() { return static_cast<uint8_t>(ElementType::Empty); }

    static constexpr uint8_t SolidMaterial() { return static_cast<uint8_t>(ElementType::Sand); }

    void SetBlock(int x, int y, uint8_t material)
    {
        generatedWorld[static_cast<size_t>(y) * static_cast<size_t>(m_width) +
                       static_cast<size_t>(x)] = material;
    }

    float GetFBm(FastNoiseLite &noise, float x, float y, int octaves) const
    {
        float total     = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maximum   = 0.0f;

        for (int octave = 0; octave < octaves; ++octave)
        {
            total += noise.GetNoise(x * frequency, y * frequency) * amplitude;

            maximum += amplitude;
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        if (maximum == 0.0f)
        {
            return 0.0f;
        }

        return total / maximum;
    }

    void ApplyCellularAutomata(const Config &config)
    {
        const int blockSize = std::max(1, static_cast<int>(config.blockSize));

        const int skySkipLimit =
            static_cast<int>(static_cast<float>(m_height) * config.surfaceRatio) - 150;

        for (int iteration = 0; iteration < config.caIterations; ++iteration)
        {
            // Read from generatedWorld and write to this buffer.
            std::vector<uint8_t> buffer = generatedWorld;

            for (int y = 0; y < m_height; y += blockSize)
            {
                for (int x = 0; x < m_width; x += blockSize)
                {
                    const uint8_t currentState = GetBlock(x, y);

                    // Skip empty blocks high above the surface.
                    if (currentState == EmptyMaterial() && y < skySkipLimit)
                    {
                        continue;
                    }

                    int wallCount = 0;

                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        for (int dx = -1; dx <= 1; ++dx)
                        {
                            if (dx == 0 && dy == 0)
                            {
                                continue;
                            }

                            const int neighborX = x + dx * blockSize;

                            const int neighborY = y + dy * blockSize;

                            if (neighborX < 0 || neighborX >= m_width || neighborY < 0 ||
                                neighborY >= m_height)
                            {
                                ++wallCount;
                            }
                            else if (GetBlock(neighborX, neighborY) == SolidMaterial())
                            {
                                ++wallCount;
                            }
                        }
                    }

                    uint8_t newState = currentState;

                    if (currentState == SolidMaterial())
                    {
                        // Remove isolated solid blocks.
                        if (wallCount < 4)
                        {
                            newState = EmptyMaterial();
                        }
                    }
                    else
                    {
                        // Fill isolated empty blocks.
                        if (wallCount >= 5)
                        {
                            newState = SolidMaterial();
                        }
                    }

                    // Apply the state to the complete block.
                    for (int blockY = 0; blockY < blockSize; ++blockY)
                    {
                        for (int blockX = 0; blockX < blockSize; ++blockX)
                        {
                            const int targetX = x + blockX;

                            const int targetY = y + blockY;

                            if (targetX >= m_width || targetY >= m_height)
                            {
                                continue;
                            }

                            buffer[static_cast<size_t>(targetY) * static_cast<size_t>(m_width) +
                                   static_cast<size_t>(targetX)] = newState;
                        }
                    }
                }
            }

            generatedWorld.swap(buffer);
        }
    }
};
