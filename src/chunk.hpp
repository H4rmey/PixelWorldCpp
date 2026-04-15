#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "element.hpp"
#include "helpers/constants.h"
#include "helpers/move_decision.h"
#include "movement/IMovement.h"

class WorldManager;

// ---------------------------------------------------------
// Neighbor directions
// ---------------------------------------------------------
enum class NeighborDir : uint8_t {
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

struct Rect {
    int min_x = 0;
    int min_y = 0;
    int max_x = -1;
    int max_y = -1;

    bool is_valid() const { return max_x >= min_x && max_y >= min_y; }
};

class Chunk {
  public:
    static constexpr int m_width  = CHUNK_SIZE_X;
    static constexpr int m_height = CHUNK_SIZE_Y;

    uint32_t active_index = UINT32_MAX;

  private:
    int m_world_pos_x = -2;
    int m_world_pos_y = -2;

    std::vector<Element> m_elements{};

    mutable std::vector<MoveDecision> m_decisions;

    Rect m_dirty_rect{};

    WorldManager *m_world;

  public:
    Chunk()  = default;
    ~Chunk() = default;

    Chunk(const Chunk &)            = delete;
    Chunk &operator=(const Chunk &) = delete;

    Chunk(Chunk &&)            = default;
    Chunk &operator=(Chunk &&) = default;

    void init(WorldManager *world, int x, int y) {
        m_world       = world;
        m_world_pos_x = x;
        m_world_pos_y = y;
        m_decisions.reserve(m_width * m_height);
        m_elements.resize(m_width * m_height);
        fill_with_empty();
    }

    inline static EmptyMovement empty_movement;

    void fill_with_empty();

    inline const Rect    &get_dirty_rect() const { return m_dirty_rect; }
    inline static int     index(int x, int y) { return m_width * y + x; }
    inline Element       &at(int x, int y) { return m_elements[index(x, y)]; }
    inline const Element &at(int x, int y) const {
        return m_elements[index(x, y)];
    }
    inline const int &get_world_pos_x() const { return m_world_pos_x; }
    inline const int &get_world_pos_y() const { return m_world_pos_y; }

    void set_element_local(int x, int y, const Element &e);

    inline void mark_dirty(int x, int y) {
        if (!m_dirty_rect.is_valid()) {
            m_dirty_rect = {x, y, x, y};
        } else {
            m_dirty_rect.min_x = std::min(m_dirty_rect.min_x, x);
            m_dirty_rect.min_y = std::min(m_dirty_rect.min_y, y);
            m_dirty_rect.max_x = std::max(m_dirty_rect.max_x, x);
            m_dirty_rect.max_y = std::max(m_dirty_rect.max_y, y);
        }
    }
    inline void clear_dirty() {
        m_dirty_rect.min_x = 0;
        m_dirty_rect.min_y = 0;
        m_dirty_rect.max_x = -1;
        m_dirty_rect.max_y = -1;
    }

    inline bool is_dirty() const {
        return m_dirty_rect.max_x >= m_dirty_rect.min_x &&
               m_dirty_rect.max_y >= m_dirty_rect.min_y;
    }

    inline const Rect &get_dirty_rect_local() const { return m_dirty_rect; }

    const Element             &get_element_global(int x, int y) const;
    const Element             &get_element_local(int x, int y) const;
    std::vector<MoveDecision> &process_movement(bool bias);
    std::vector<MoveDecision>
    apply_decisions(const std::vector<MoveDecision> &decisions);
};
