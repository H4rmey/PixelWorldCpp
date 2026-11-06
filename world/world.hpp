#pragma once

#include "chunkManager.hpp"
#include "elementFactory.hpp"
#include "worldGenerator.hpp"

class World
{
  private:
    WorldGenerator generator;

  public:
    ChunkManager chunkManager;

    World() = default;

    void init()
    {
        WorldGenerator::Config config;

        printf("Start: generating world\n");
        generator.Generate(config);
        generator.ExportImage("world.ppm");
        printf("Start: setting pixels\n");

        chunkManager.init();
        for (int i = 0; i < generator.generatedWorld.size(); i++)
        {
            Coord   coord = index_to_world_coords(i);
            Element element;
            if (generator.generatedWorld[i] == 0)
            {
                element = ElementFactory::create_empty();
            }
            else
            {
                element = ElementFactory::create_sand();
            }
            chunkManager.set_element_global(coord.x, coord.y, element);
        }

        printf("Start: deactivating chunks\n");
        const std::vector<Chunk *> chunks = chunkManager.get_active_chunks();
        for (Chunk *chunk : chunks)
        {
            chunkManager.deactivate_chunk(*chunk);
        }
    }
};
