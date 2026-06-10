#include "world_manager.hpp"
#include "helpers/constants.h"
#include "helpers/move_decision.h"
#include "helpers/tools.h"
#include <vector>

Chunk &WorldManager::chunk_at(int x, int y)
{
    if (x < 0 || x >= CHUNK_COUNT_X || y < 0 || y >= CHUNK_COUNT_Y)
    {
        return m_bedrock_chunk;
    }

    return m_chunks[y * CHUNK_COUNT_X + x];
}

const Chunk &WorldManager::chunk_at(int x, int y) const
{
    if (x < 0 || x >= CHUNK_COUNT_X || y < 0 || y >= CHUNK_COUNT_Y)
    {
        return m_bedrock_chunk;
    }

    return m_chunks[y * CHUNK_COUNT_X + x];
}

void WorldManager::activate_chunk(Chunk &c)
{
    if (c.active_index != UINT32_MAX)
        return; // already active

    c.active_index = static_cast<uint32_t>(m_active_chunks.size());
    m_active_chunks.push_back(&c);
}

void WorldManager::deactivate_chunk(Chunk &c)
{
    uint32_t idx = c.active_index;
    if (idx == UINT32_MAX)
        return; // not active

    Chunk *last          = m_active_chunks.back();
    m_active_chunks[idx] = last;
    last->active_index   = idx;

    m_active_chunks.pop_back();
    c.active_index = UINT32_MAX;
}

Element &WorldManager::get_element_global(int x, int y)
{
    // Determine which chunk the global coordinate belongs to
    int chunk_x = x / CHUNK_SIZE_X;
    int chunk_y = y / CHUNK_SIZE_Y;

    // Convert to local chunk coordinates
    int local_x = x % CHUNK_SIZE_X;
    int local_y = y % CHUNK_SIZE_Y;

    // Handle negative modulo (C++ keeps negative remainders)
    if (local_x < 0)
        local_x += CHUNK_SIZE_X;
    if (local_y < 0)
        local_y += CHUNK_SIZE_Y;

    // Bounds check
    if (chunk_x < 0 || chunk_x >= CHUNK_COUNT_X || chunk_y < 0 ||
        chunk_y >= CHUNK_COUNT_Y)
    {
        throw std::out_of_range("Global coordinates out of world bounds");
    }

    // Convert chunk coordinates to index
    int chunk_index = chunk_y * CHUNK_COUNT_X + chunk_x;

    // Return the element inside the chunk
    return m_chunks[chunk_index].at(local_x, local_y);
}

bool WorldManager::get_chunk_for_global_cell(int gx, int gy, int &out_x,
                                             int &out_y, int &out_lx,
                                             int &out_ly) const
{
    if (gx < 0 || gy < 0)
        return false;

    int cells_per_chunk_x = Chunk::m_width;
    int cells_per_chunk_y = Chunk::m_height;

    out_x = gx / cells_per_chunk_x;
    out_y = gy / cells_per_chunk_y;

    if (out_x < 0 || out_x >= CHUNK_COUNT_X || out_y < 0 ||
        out_y >= CHUNK_COUNT_Y)
        return false;

    out_lx = gx % cells_per_chunk_x;
    out_ly = gy % cells_per_chunk_y;
    return true;
}

void WorldManager::step()
{
    if (tick >= GRAVITY)
    {
        tick = 0;
    }
    // Freeze the active set for this tick.
    // Cross-chunk swaps may activate additional chunks; those will run next
    const size_t active_count = m_active_chunks.size();
    if (active_count == 0)
        return;

    // Phase buffers sized to the frozen count
    m_decision_ptrs.clear();
    m_decision_ptrs.reserve(active_count);

    m_chunks_to_deactivate.clear();
    m_chunks_to_deactivate.reserve(active_count / 2);

    static thread_local std::vector<uint8_t> had_decisions;
    had_decisions.assign(active_count, 0);

    //  collect decisions (NO WRITES)
    for (size_t i = 0; i < active_count; ++i)
    {
        Chunk *c = m_active_chunks[i];

        std::vector<MoveDecision> &decisions =
            c->process_movement(tick, m_bias);
        m_decision_ptrs.push_back(&decisions);

        had_decisions[i] = static_cast<uint8_t>(!decisions.empty());

        // Consume the dirty rect used for scanning.
        // Any swaps will mark dirty again for the next tick.
        c->clear_dirty();
    }

    //  apply in-chunk decisions
    m_cross.clear();

    for (size_t i = 0; i < active_count; ++i)
    {
        Chunk                     *c         = m_active_chunks[i];
        std::vector<MoveDecision> &decisions = *m_decision_ptrs[i];

        std::vector<MoveDecision> outside = c->apply_decisions(decisions);

        if (!outside.empty())
            m_cross.insert(m_cross.end(), outside.begin(), outside.end());
    }

    //  resolve cross-chunk swaps
    // (may activate more chunks; that's OK now)
    if (!m_cross.empty())
        apply_cross_chunk_swaps(m_cross);

    //  deactivate stable chunks from the frozen set only
    for (size_t i = 0; i < active_count; ++i)
    {
        Chunk *c = m_active_chunks[i];

        // Deactivate only if:
        //  - it produced no decisions in Phase 1
        //  - AND nothing dirtied it during apply/cross swap
        if (!had_decisions[i] && !c->is_dirty())
            m_chunks_to_deactivate.push_back(c);
    }

    for (Chunk *c : m_chunks_to_deactivate)
    {
        deactivate_chunk(*c);

        // IMPORTANT: do NOT clear dirty here.
        // Dirty should represent "needs work next tick".
    }
    m_bias = !m_bias;
    tick++;
}

void WorldManager::apply_cross_chunk_swaps(
    const std::vector<MoveDecision> &cross)
{
    constexpr int W               = Chunk::m_width;
    constexpr int H               = Chunk::m_height;
    const int     cells_per_chunk = W * H;

    // Per-chunk used masks (byte-per-cell, fast)
    static std::vector<uint8_t> used;
    used.assign(CHUNK_COUNT_X * CHUNK_COUNT_Y * cells_per_chunk, 0);

    auto used_for = [&](int cx, int cy) -> uint8_t *
    { return used.data() + (cy * CHUNK_COUNT_X + cx) * cells_per_chunk; };

    for (const MoveDecision &d : cross)
    {
        if (d.move_type != MoveType::Swap &&
            d.move_type != MoveType::Processing)
            continue;

        //  Convert local coords (possibly negative/oob) to GLOBAL
        const int base_gx = d.origin_cx * W;
        const int base_gy = d.origin_cy * H;

        const int old_gx = base_gx + d.old_x;
        const int old_gy = base_gy + d.old_y;
        const int new_gx = base_gx + d.new_x;
        const int new_gy = base_gy + d.new_y;

        //  Map GLOBAL -> owning chunk + local coords (works for negatives)
        const int old_cx = floor_div(old_gx, W);
        const int old_cy = floor_div(old_gy, H);
        const int new_cx = floor_div(new_gx, W);
        const int new_cy = floor_div(new_gy, H);

        // If you only simulate a fixed grid of chunks, skip moves that leave
        // the world:
        if ((unsigned)old_cx >= (unsigned)CHUNK_COUNT_X ||
            (unsigned)old_cy >= (unsigned)CHUNK_COUNT_Y ||
            (unsigned)new_cx >= (unsigned)CHUNK_COUNT_X ||
            (unsigned)new_cy >= (unsigned)CHUNK_COUNT_Y)
        {
            continue;
        }

        const int old_lx = floor_mod(old_gx, W);
        const int old_ly = floor_mod(old_gy, H);
        const int new_lx = floor_mod(new_gx, W);
        const int new_ly = floor_mod(new_gy, H);

        //  Conflict protection (each cell max once)
        uint8_t *used_old = used_for(old_cx, old_cy);
        uint8_t *used_new = used_for(new_cx, new_cy);

        const int old_i = Chunk::index(old_lx, old_ly);
        const int new_i = Chunk::index(new_lx, new_ly);

        if (used_old[old_i] || used_new[new_i])
        {
            // conflict policy: skip (fastest). alternative: retry list.
            continue;
        }

        used_old[old_i] = 1;
        used_new[new_i] = 1;

        //  Apply swap across chunks
        Chunk &c_old = chunk_at(old_cx, old_cy);
        Chunk &c_new = chunk_at(new_cx, new_cy);

        Element tmp = c_old.at(old_lx, old_ly);
        c_old.set_element_local(old_lx, old_ly, c_new.at(new_lx, new_ly));
        c_new.set_element_local(new_lx, new_ly, tmp);

        c_old.mark_dirty(old_lx, old_ly);
        c_new.mark_dirty(new_lx, new_ly);
    }
}
