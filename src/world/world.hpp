#pragma once

#include "chunkManager.hpp"
#include "elementFactory.hpp"
#include "world/helpers/renderLayer.h"
#include "worldGenerator.hpp"
#include <system_error>

class World
{
  private:
    WorldGenerator generator;

  public:
    ChunkManager chunkManager;

    World() {}

    void init()
    {
        WorldGenerator::Config config;

        chunkManager = ChunkManager();
        generator    = WorldGenerator();
        generator.Generate(config);

        for (int i = 0; i < generator.generatedWorld.size(); i++)
        {
            Coord   c = index_to_world_coords(i);
            Element e = ElementFactory::create_sand();
            chunkManager.set_element_global(c.x, c.y, e);
        }
    }

    void step() { chunkManager.step(); }
};
