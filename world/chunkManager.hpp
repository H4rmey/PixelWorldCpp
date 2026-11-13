#pragma once

#include <array>
#include <cstdint>

#include "chunk.hpp"
#include "element.hpp"
#include "helpers/constants.h"

class ChunkManager
{
  private:
    static constexpr std::size_t m_size = CHUNK_COUNT;

    std::array<Chunk, CHUNK_COUNT>           m_chunks;
    std::vector<std::vector<MoveDecision> *> m_decision_ptrs;
    std::vector<MoveDecision>                m_cross;
    std::vector<Chunk *>                     m_active_chunks;
    std::vector<Chunk *>                     m_chunks_to_deactivate;
    std::vector<Point>                       m_changed_pixels;

    Chunk   m_bedrock_chunk;
    Element m_empty_element;

    bool m_bias = true;

  public:
    ChunkManager() = default;

    void init();

    Chunk       &chunk_at(int cx, int cy);
    const Chunk &chunk_at(int cx, int cy) const;

    inline static int index(int x, int y) { return CHUNK_COUNT_X * y + x; }

    void activate_chunk(Chunk &c);
    void deactivate_chunk(Chunk &c);
    void process_activations();

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
