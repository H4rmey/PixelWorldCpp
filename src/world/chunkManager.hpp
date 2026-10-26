#pragma once

#include <array>
#include <cstdint>

#include "chunk.hpp"
#include "element.hpp"
#include "helpers/constants.h"
#include "helpers/element_type.h"

class ChunkManager
{
  private:
    static constexpr std::size_t m_size = CHUNK_COUNT;

    std::array<Chunk, CHUNK_COUNT>           m_chunks;
    std::vector<Chunk *>                     m_active_chunks;
    std::vector<std::vector<MoveDecision> *> m_decision_ptrs;
    std::vector<MoveDecision>                m_cross;
    std::vector<Chunk *>                     m_chunks_to_deactivate;

    Chunk m_bedrock_chunk;

    bool m_bias = true;

  public:
    ChunkManager()
    {
        // initialize the bedrock chunk
        m_bedrock_chunk = Chunk();
        m_bedrock_chunk.init(this, -1, -1);
        for (int y = 0; y < CHUNK_COUNT_Y; ++y)
        {
            for (int x = 0; x < CHUNK_COUNT_X; ++x)
            {
                Chunk &chunk = m_chunks[index(x, y)];
                chunk.init(this, x, y);

                if (x == 0 || y == 0 || x == CHUNK_COUNT_X - 1 || y == CHUNK_COUNT_Y - 1)
                {
                    for (int ly = 0; ly < CHUNK_SIZE_Y; ++ly)
                    {
                        for (int lx = 0; lx < CHUNK_SIZE_X; ++lx)
                        {
                            chunk.at(lx, ly).type = ElementType::Bedrock;
                        }
                    }
                    // optionally: chunk.deactivate / don't activate ever
                }
            }
        }

        // now initicalize the other chunks
        for (int y = 0; y < CHUNK_COUNT_Y; y++)
        {
            for (int x = 0; x < CHUNK_COUNT_X; x++)
            {
                Chunk &chunk = m_chunks[index(x, y)];
                chunk.init(this, x, y);
            }
        }
    }

    Chunk       &chunk_at(int cx, int cy);
    const Chunk &chunk_at(int cx, int cy) const;

    inline static int index(int x, int y) { return CHUNK_COUNT_X * y + x; }

    void activate_chunk(Chunk &c);
    void deactivate_chunk(Chunk &c);

    void                        set_element_global(int x, int y, Element &e);
    Element                    &get_element_global(int x, int y);
    const Element              &get_element_global(int x, int y) const;
    const std::vector<Chunk *> &get_active_chunks() const { return m_active_chunks; }

    // Map global cell coordinates to chunk + local coords
    bool get_chunk_for_global_cell(int gx, int gy, int &out_x, int &out_y, int &out_lx,
                                   int &out_ly) const;

    // One full simulation step
    void step();

    void apply_cross_chunk_swaps(const std::vector<MoveDecision> &cross);

    uint8_t tick = 0;
};
