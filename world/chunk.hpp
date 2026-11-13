#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "element.hpp"
#include "helpers/constants.h"
#include "helpers/move_decision.h"
#include "movement/IMovement.h"

// ---------------------------------------------------------
// Neighbor directions
// ---------------------------------------------------------
enum class NeighborDir : uint8_t
{
    Left = 0,
    Right,
    Up,
    Down,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight,
    COUNT
};

struct Rect
{
    int min_x = 0;
    int min_y = 0;
    int max_x = -1;
    int max_y = -1;

    bool is_valid() const { return max_x >= min_x && max_y >= min_y; }
};

class Chunk
{
  public:
    static constexpr int m_width  = CHUNK_SIZE_X;
    static constexpr int m_height = CHUNK_SIZE_Y;

    uint32_t active_index = UINT32_MAX;

    // Pointers to directly adjacent chunks, decoupled from the Manager
    Chunk *neighbors[static_cast<int>(NeighborDir::COUNT)]{nullptr};
    bool   needs_activation = false;

  private:
    int m_world_pos_x = -2;
    int m_world_pos_y = -2;

    std::vector<Element> m_elements{};

    mutable std::vector<MoveDecision> m_decisions;

    Rect m_dirty_rect{};

    Element bedrockElement;

  public:
    Chunk()  = default;
    ~Chunk() = default;

    Chunk(const Chunk &)            = delete;
    Chunk &operator=(const Chunk &) = delete;

    Chunk(Chunk &&)            = default;
    Chunk &operator=(Chunk &&) = default;

    void init(int x, int y);

    void fill_with_empty();

    void activate_cardinal_neighbors();
    void activate_all_neighbors();

    inline const Rect &get_dirty_rect() const { return m_dirty_rect; }
    inline static int  index(int local_x, int local_y) { return m_width * local_y + local_x; }
    inline Element    &at(int local_x, int local_y) { return m_elements[index(local_x, local_y)]; }
    inline const Element &at(int local_x, int local_y) const
    {
        return m_elements[index(local_x, local_y)];
    }
    inline const int &get_world_pos_x() const { return m_world_pos_x; }
    inline const int &get_world_pos_y() const { return m_world_pos_y; }

    void set_element_local(int local_x, int local_y, const Element &e);
    void set_element_global(int global_x, int global_y, const Element &e);

    inline void mark_dirty(int local_x, int local_y)
    {
        if (!m_dirty_rect.is_valid())
        {
            m_dirty_rect = {local_x, local_y, local_x, local_y};
        }
        else
        {
            m_dirty_rect.min_x = std::min(m_dirty_rect.min_x, local_x);
            m_dirty_rect.min_y = std::min(m_dirty_rect.min_y, local_y);
            m_dirty_rect.max_x = std::max(m_dirty_rect.max_x, local_x);
            m_dirty_rect.max_y = std::max(m_dirty_rect.max_y, local_y);
        }
    }
    inline void clear_dirty()
    {
        m_dirty_rect.min_x = 0;
        m_dirty_rect.min_y = 0;
        m_dirty_rect.max_x = -1;
        m_dirty_rect.max_y = -1;
    }

    inline bool is_dirty() const
    {
        return m_dirty_rect.max_x >= m_dirty_rect.min_x && m_dirty_rect.max_y >= m_dirty_rect.min_y;
    }

    inline const Rect &get_dirty_rect_local() const { return m_dirty_rect; }

    Element &get_element_global(int global_x, int global_y);
    Element &get_element_local(int local_x, int local_y);

    void get_global_coords(int local_x, int local_y, int &out_global_x, int &out_global_y) const;

    std::vector<MoveDecision> &process_movement(uint8_t tick, bool bias);

    void apply_decisions(std::vector<MoveDecision> &decisions,
                         std::vector<MoveDecision> &out_of_bounds,
                         std::vector<Point>        &changed_pixels);
};
