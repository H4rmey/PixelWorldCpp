#include "chunk.hpp"
#include "elementFactory.hpp"
#include "helpers/constants.h"
#include "helpers/move_decision.h"
#include "movement/IMovement.h"
#include <random>
#include <raylib.h>

void Chunk::fill_with_empty()
{
    for (Element &e : m_elements)
    {
        e = ElementFactory::create_empty();
    }
}

void Chunk::init(int x, int y)
{
    m_world_pos_x = x;
    m_world_pos_y = y;

    bedrockElement = ElementFactory::create_bedrock();

    m_decisions.reserve(m_width * m_height);
    m_elements.resize(m_width * m_height);

    fill_with_empty();
}

// ---------------------------------------------------------
// Neighbor Activation Logic
// ---------------------------------------------------------

void Chunk::activate_cardinal_neighbors()
{
    if (neighbors[static_cast<int>(NeighborDir::Left)])
    {
        neighbors[static_cast<int>(NeighborDir::Left)]->needs_activation = true;
    }
    if (neighbors[static_cast<int>(NeighborDir::Right)])
    {
        neighbors[static_cast<int>(NeighborDir::Right)]->needs_activation = true;
    }
    if (neighbors[static_cast<int>(NeighborDir::Up)])
    {
        neighbors[static_cast<int>(NeighborDir::Up)]->needs_activation = true;
    }
    if (neighbors[static_cast<int>(NeighborDir::Down)])
    {
        neighbors[static_cast<int>(NeighborDir::Down)]->needs_activation = true;
    }
}

void Chunk::activate_all_neighbors()
{
    for (int i = 0; i < static_cast<int>(NeighborDir::COUNT); ++i)
    {
        if (neighbors[i])
        {
            neighbors[i]->needs_activation = true;
        }
    }
}

void Chunk::set_element_local(int x, int y, const Element &e)
{
    m_elements[index(x, y)] = e;
    needs_activation        = true;

    if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1)
    {
        activate_cardinal_neighbors();
    }

    // Left border affects left neighbor's right border
    if (x == 0 && neighbors[static_cast<int>(NeighborDir::Left)])
    {
        Chunk &left           = *neighbors[static_cast<int>(NeighborDir::Left)];
        left.needs_activation = true;
        left.mark_dirty(Chunk::m_width - 1, y);
    }

    // Right border affects right neighbor's left border
    if (x == m_width - 1 && neighbors[static_cast<int>(NeighborDir::Right)])
    {
        Chunk &right           = *neighbors[static_cast<int>(NeighborDir::Right)];
        right.needs_activation = true;
        right.mark_dirty(0, y);
    }

    // Top border affects upper neighbor's bottom border
    if (y == 0 && neighbors[static_cast<int>(NeighborDir::Up)])
    {
        Chunk &up           = *neighbors[static_cast<int>(NeighborDir::Up)];
        up.needs_activation = true;
        up.mark_dirty(x, Chunk::m_height - 1);
    }

    // Bottom border affects lower neighbor's top border
    if (y == m_height - 1 && neighbors[static_cast<int>(NeighborDir::Down)])
    {
        Chunk &down           = *neighbors[static_cast<int>(NeighborDir::Down)];
        down.needs_activation = true;
        down.mark_dirty(x, 0);
    }

    mark_dirty(x, y);
}

// Returns the global coordinate of a specific local tile
void Chunk::get_global_coords(int local_x, int local_y, int &out_global_x, int &out_global_y) const
{
    out_global_x = m_world_pos_x * m_width + local_x;
    out_global_y = m_world_pos_y * m_height + local_y;
}

Element &Chunk::get_element_local(int x, int y)
{
    // Fast path (very common case)
    if ((unsigned)x < m_width && (unsigned)y < m_height)
    {
        return m_elements[y * m_width + x];
    }

    // Compute neighbor offsets (branch-light)
    const int nx = (x < 0) ? -1 : (x >= m_width ? 1 : 0);
    const int ny = (y < 0) ? -1 : (y >= m_height ? 1 : 0);

    NeighborDir dir;
    if (nx == -1 && ny == 0)
    {
        dir = NeighborDir::Left;
    }
    else if (nx == 1 && ny == 0)
    {
        dir = NeighborDir::Right;
    }
    else if (nx == 0 && ny == -1)
    {
        dir = NeighborDir::Up;
    }
    else if (nx == 0 && ny == 1)
    {
        dir = NeighborDir::Down;
    }
    else if (nx == -1 && ny == -1)
    {
        dir = NeighborDir::UpLeft;
    }
    else if (nx == 1 && ny == -1)
    {
        dir = NeighborDir::UpRight;
    }
    else if (nx == -1 && ny == 1)
    {
        dir = NeighborDir::DownLeft;
    }
    else
    {
        dir = NeighborDir::DownRight;
    }

    // Wrap coordinates (±1 only case)
    const int local_x = (x < 0) ? (m_width - 1) : (x >= m_width) ? 0 : x;
    const int local_y = (y < 0) ? (m_height - 1) : (y >= m_height) ? 0 : y;

    Chunk *neighbor = neighbors[static_cast<int>(dir)];
    if (neighbor)
    {
        return neighbor->m_elements[local_y * m_width + local_x];
    }
    return bedrockElement;
}

Element &Chunk::get_element_global(int global_x, int global_y)
{
    if (global_x < 0 || global_y < 0 || global_x >= WORLD_SIZE_X || global_y >= WORLD_SIZE_Y)
    {
        return bedrockElement;
    }
    // Convert global → local
    int local_x = global_x - m_world_pos_x * m_width;
    int local_y = global_y - m_world_pos_y * m_height;

    // If inside this chunk, return directly
    if (local_x >= 0 && local_x < m_width && local_y >= 0 && local_y < m_height)
    {
        return m_elements[local_y * m_width + local_x];
    }

    // Otherwise, recursively step to the correct chunk
    int target_chunk_x = global_x / m_width;
    int target_chunk_y = global_y / m_height;

    int nx = (target_chunk_x > m_world_pos_x) ? 1 : (target_chunk_x < m_world_pos_x ? -1 : 0);
    int ny = (target_chunk_y > m_world_pos_y) ? 1 : (target_chunk_y < m_world_pos_y ? -1 : 0);

    NeighborDir dir;
    if (nx == -1 && ny == 0)
    {
        dir = NeighborDir::Left;
    }
    else if (nx == 1 && ny == 0)
    {
        dir = NeighborDir::Right;
    }
    else if (nx == 0 && ny == -1)
    {
        dir = NeighborDir::Up;
    }
    else if (nx == 0 && ny == 1)
    {
        dir = NeighborDir::Down;
    }
    else if (nx == -1 && ny == -1)
    {
        dir = NeighborDir::UpLeft;
    }
    else if (nx == 1 && ny == -1)
    {
        dir = NeighborDir::UpRight;
    }
    else if (nx == -1 && ny == 1)
    {
        dir = NeighborDir::DownLeft;
    }
    else
    {
        dir = NeighborDir::DownRight;
    }

    Chunk *neighbor = neighbors[static_cast<int>(dir)];
    if (neighbor)
    {
        return neighbor->get_element_global(global_x, global_y);
    }
    return bedrockElement;
}

std::vector<MoveDecision> &Chunk::process_movement(uint8_t tick, bool bias)
{
    m_decisions.clear();

    for (int y = m_dirty_rect.max_y + 3; y >= m_dirty_rect.min_y - 3; --y)
    {
        if (bias)
        {
            for (int x = m_dirty_rect.max_x + 3; x >= m_dirty_rect.min_x - 3; --x)
            {
                if (y < 0 || x < 0 || x >= CHUNK_SIZE_X || y >= CHUNK_SIZE_Y)
                {
                    continue;
                }

                int              i = index(x, y);
                Element          e = m_elements[index(x, y)];
                const IMovement *m = e.movement;
                MoveDecision     d = m->process(tick, x, y, *this);

                if (d.move_type != MoveType::Stay)
                {
                    d.origin_cx = m_world_pos_x;
                    d.origin_cy = m_world_pos_y;
                    m_decisions.push_back(d);
                }
            }
        }
        else
        {
            for (int x = m_dirty_rect.min_x - 3; x <= m_dirty_rect.max_x + 3; ++x)
            {
                if (y < 0 || x < 0 || x >= CHUNK_SIZE_X || y >= CHUNK_SIZE_Y)
                {
                    continue;
                }

                MoveDecision d = m_elements[index(x, y)].movement->process(tick, x, y, *this);

                if (d.move_type != MoveType::Stay)
                {
                    d.origin_cx = m_world_pos_x;
                    d.origin_cy = m_world_pos_y;
                    m_decisions.push_back(d);
                }
            }
        }
    }

    return m_decisions;
}

std::vector<MoveDecision> Chunk::apply_decisions(std::vector<MoveDecision> &decisions)
{
    // 1. Initialize the random engine
    static std::random_device rd;
    static std::mt19937       g(rd());

    // 2. Shuffle the input vector directly
    std::shuffle(decisions.begin(), decisions.end(), g);

    // 3. Prepare the return vector
    std::vector<MoveDecision> out_of_bounds;
    out_of_bounds.reserve(decisions.size() / 8);

    static thread_local std::vector<uint8_t> used;
    used.assign(m_width * m_height, 0);

    // ---- helper to wake neighbor if a BORDER cell changed ----
    auto notify_border_change = [&](int x, int y)
    {
        // Left edge changed -> left neighbor needs its right edge dirty
        if (x == 0 && neighbors[static_cast<int>(NeighborDir::Left)])
        {
            Chunk &n           = *neighbors[static_cast<int>(NeighborDir::Left)];
            n.needs_activation = true;
            n.mark_dirty(Chunk::m_width - 1, y);
        }

        // Right edge changed -> right neighbor needs its left edge dirty
        if (x == m_width - 1 && neighbors[static_cast<int>(NeighborDir::Right)])
        {
            Chunk &n           = *neighbors[static_cast<int>(NeighborDir::Right)];
            n.needs_activation = true;
            n.mark_dirty(0, y);
        }

        // Top edge changed -> upper neighbor needs its bottom edge dirty
        if (y == 0 && neighbors[static_cast<int>(NeighborDir::Up)])
        {
            Chunk &n           = *neighbors[static_cast<int>(NeighborDir::Up)];
            n.needs_activation = true;
            n.mark_dirty(x, Chunk::m_height - 1);
        }

        // Bottom edge changed -> lower neighbor needs its top edge dirty
        if (y == m_height - 1 && neighbors[static_cast<int>(NeighborDir::Down)])
        {
            Chunk &n           = *neighbors[static_cast<int>(NeighborDir::Down)];
            n.needs_activation = true;
            n.mark_dirty(x, 0);
        }
    };
    // ------------------------------------------------------------

    for (const MoveDecision &d : decisions)
    {
        if (d.move_type != MoveType::Swap && d.move_type != MoveType::Processing)
        {
            continue;
        }

        const bool old_in =
            (unsigned)d.old_x < (unsigned)m_width && (unsigned)d.old_y < (unsigned)m_height;
        const bool new_in =
            (unsigned)d.new_x < (unsigned)m_width && (unsigned)d.new_y < (unsigned)m_height;

        if (!old_in || !new_in)
        {
            out_of_bounds.push_back(d);
            continue;
        }

        const uint16_t oi = (uint16_t)index(d.old_x, d.old_y);
        const uint16_t ni = (uint16_t)index(d.new_x, d.new_y);

        if (used[oi] || used[ni])
        {
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
