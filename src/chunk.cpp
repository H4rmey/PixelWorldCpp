#include "chunk.hpp"
#include "helpers/constants.h"
#include "world_manager.hpp"
#include <iostream>
#include <raylib.h>

void Chunk::fill_with_empty() {
    for (Element &e : m_elements) {
        e = Element(ElementType::Empty, &empty_movement);
    }
}

void Chunk::set_element_local(int x, int y, const Element &e) {
    m_elements[index(x, y)] = e;
    m_world->activate_chunk(*this);

    int cx = m_world_pos_x;
    int cy = m_world_pos_y;

    if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1) {
        if (cx > 0)
            m_world->activate_chunk(m_world->chunk_at(cx - 1, cy));
        if (cx < CHUNK_COUNT_X - 1)
            m_world->activate_chunk(m_world->chunk_at(cx + 1, cy));
        if (cy > 0)
            m_world->activate_chunk(m_world->chunk_at(cx, cy - 1));
        if (cy < CHUNK_COUNT_Y - 1)
            m_world->activate_chunk(m_world->chunk_at(cx, cy + 1));
    }

    // Left border affects left neighbor's right border
    if (x == 0 && cx > 0) {
        Chunk &left = m_world->chunk_at(cx - 1, cy);
        m_world->activate_chunk(left);
        left.mark_dirty(Chunk::m_width - 1, y);
    }

    // Right border affects right neighbor's left border
    if (x == m_width - 1 && cx < CHUNK_COUNT_X - 1) {
        Chunk &right = m_world->chunk_at(cx + 1, cy);
        m_world->activate_chunk(right);
        right.mark_dirty(0, y);
    }

    // Top border affects upper neighbor's bottom border
    if (y == 0 && cy > 0) {
        Chunk &up = m_world->chunk_at(cx, cy - 1);
        m_world->activate_chunk(up);
        up.mark_dirty(x, Chunk::m_height - 1);
    }

    // Bottom border affects lower neighbor's top border
    if (y == m_height - 1 && cy < CHUNK_COUNT_Y - 1) {
        Chunk &down = m_world->chunk_at(cx, cy + 1);
        m_world->activate_chunk(down);
        down.mark_dirty(x, 0);
    }

    mark_dirty(x, y);
}

const Element &Chunk::get_element_local(int x, int y) const {
    // Fast path (very common case)
    if ((unsigned)x < m_width && (unsigned)y < m_height) {
        return m_elements[y * m_width + x];
    }

    // Compute neighbor offsets (branch-light)
    const int nx = (x < 0) ? -1 : (x >= m_width);
    const int ny = (y < 0) ? -1 : (y >= m_height);

    Chunk &neighbor = m_world->chunk_at(m_world_pos_x + nx, m_world_pos_y + ny);

    // Wrap coordinates (±1 only case)
    const int local_x = (x < 0) ? (m_width - 1) : (x >= m_width) ? 0 : x;

    const int local_y = (y < 0) ? (m_height - 1) : (y >= m_height) ? 0 : y;

    return neighbor.m_elements[local_y * m_width + local_x];
}

const Element &Chunk::get_element_global(int global_x, int global_y) const {
    // Convert global → local
    int local_x = global_x - m_world_pos_x * m_width;
    int local_y = global_y - m_world_pos_y * m_height;

    // If inside this chunk, return directly
    if (local_x >= 0 && local_x < m_width && local_y >= 0 &&
        local_y < m_height) {
        return m_elements[local_y * m_width + local_x];
    }

    // Otherwise, determine which chunk should handle it
    int target_chunk_x = global_x / m_width;
    int target_chunk_y = global_y / m_height;

    // Ask the world manager for the correct chunk
    Chunk &neighbor = m_world->chunk_at(target_chunk_x, target_chunk_y);

    // Forward the request to the correct chunk
    return neighbor.get_element_global(global_x, global_y);
}

std::vector<MoveDecision> &Chunk::process_movement(bool bias) {
    m_decisions.clear();

    // for (int y = 0; y < m_height; ++y) {
    //     for (int x = 0; x < m_width; ++x) {
    //
    for (int y = m_dirty_rect.max_y + 3; y >= m_dirty_rect.min_y - 3; --y) {
        if (bias) {
            for (int x = m_dirty_rect.max_x + 3; x >= m_dirty_rect.min_x - 3;
                 --x) {
                if (y < 0 || x < 0 || x >= CHUNK_SIZE_X || y >= CHUNK_SIZE_Y)
                    continue;

                MoveDecision d =
                    m_elements[index(x, y)].movement->process(x, y, *this);

                if (d.move_type != MoveType::Stay) {
                    d.origin_cx = m_world_pos_x;
                    d.origin_cy = m_world_pos_y;
                    m_decisions.push_back(d);
                }
            }
        } else {
            for (int x = m_dirty_rect.min_x - 3; x <= m_dirty_rect.max_x + 3;
                 ++x) {
                if (y < 0 || x < 0 || x >= CHUNK_SIZE_X || y >= CHUNK_SIZE_Y)
                    continue;

                MoveDecision d =
                    m_elements[index(x, y)].movement->process(x, y, *this);

                if (d.move_type != MoveType::Stay) {
                    d.origin_cx = m_world_pos_x;
                    d.origin_cy = m_world_pos_y;
                    m_decisions.push_back(d);
                }
            }
        }
    }

    return m_decisions;
}

std::vector<MoveDecision>
Chunk::apply_decisions(const std::vector<MoveDecision> &decisions) {
    std::vector<MoveDecision> out_of_bounds;
    out_of_bounds.reserve(decisions.size() / 8);

    static thread_local std::vector<uint8_t> used;
    used.assign(m_width * m_height, 0);

    // ---- helper to wake neighbor if a BORDER cell changed ----
    auto notify_border_change = [&](int x, int y) {
        const int cx = m_world_pos_x;
        const int cy = m_world_pos_y;

        // Left edge changed -> left neighbor needs its right edge dirty
        if (x == 0 && cx > 0) {
            Chunk &n = m_world->chunk_at(cx - 1, cy);
            m_world->activate_chunk(n);
            n.mark_dirty(Chunk::m_width - 1, y);
        }

        // Right edge changed -> right neighbor needs its left edge dirty
        if (x == m_width - 1 && cx < CHUNK_COUNT_X - 1) {
            Chunk &n = m_world->chunk_at(cx + 1, cy);
            m_world->activate_chunk(n);
            n.mark_dirty(0, y);
        }

        // Top edge changed -> upper neighbor needs its bottom edge dirty
        if (y == 0 && cy > 0) {
            Chunk &n = m_world->chunk_at(cx, cy - 1);
            m_world->activate_chunk(n);
            n.mark_dirty(x, Chunk::m_height - 1);
        }

        // Bottom edge changed -> lower neighbor needs its top edge dirty
        if (y == m_height - 1 && cy < CHUNK_COUNT_Y - 1) {
            Chunk &n = m_world->chunk_at(cx, cy + 1);
            m_world->activate_chunk(n);
            n.mark_dirty(x, 0);
        }
    };
    // ------------------------------------------------------------

    for (const MoveDecision &d : decisions) {
        if (d.move_type != MoveType::Swap)
            continue;

        const bool old_in = (unsigned)d.old_x < (unsigned)m_width &&
                            (unsigned)d.old_y < (unsigned)m_height;
        const bool new_in = (unsigned)d.new_x < (unsigned)m_width &&
                            (unsigned)d.new_y < (unsigned)m_height;

        if (!old_in || !new_in) {
            out_of_bounds.push_back(d);
            continue;
        }

        const uint16_t oi = (uint16_t)index(d.old_x, d.old_y);
        const uint16_t ni = (uint16_t)index(d.new_x, d.new_y);

        if (used[oi] || used[ni]) {
            continue;
        }

        used[oi] = 1;
        used[ni] = 1;

        std::swap(m_elements[oi], m_elements[ni]);

        mark_dirty(d.old_x, d.old_y);
        mark_dirty(d.new_x, d.new_y);

        // ---- border propagation for swaps ----
        notify_border_change(d.old_x, d.old_y);
        notify_border_change(d.new_x, d.new_y);
        // ------------------------------------------
    }

    return out_of_bounds;
}
