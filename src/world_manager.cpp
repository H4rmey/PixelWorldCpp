#include "world_manager.hpp"
#include "helpers/constants.h"
#include "helpers/move_decision.h"
#include "helpers/tools.h"

Chunk &WorldManager::chunk_at(int x, int y) {
	if (x < 0 || x >= CHUNK_COUNT_X ||
        y < 0 || y >= CHUNK_COUNT_Y)
    {
        return m_bedrock_chunk;
    }

    return m_chunks[y * CHUNK_COUNT_X + x];
}

const Chunk &WorldManager::chunk_at(int x, int y) const {
	if (x < 0 || x >= CHUNK_COUNT_X ||
        y < 0 || y >= CHUNK_COUNT_Y)
    {
        return m_bedrock_chunk;
    }

    return m_chunks[y * CHUNK_COUNT_X + x];
}

Element& WorldManager::get_element_global(int x, int y) {
    // 1. Determine which chunk the global coordinate belongs to
    int chunk_x = x / CHUNK_SIZE_X;
    int chunk_y = y / CHUNK_SIZE_Y;

    // 2. Convert to local chunk coordinates
    int local_x = x % CHUNK_SIZE_X;
    int local_y = y % CHUNK_SIZE_Y;

    // Handle negative modulo (C++ keeps negative remainders)
    if (local_x < 0) local_x += CHUNK_SIZE_X;
    if (local_y < 0) local_y += CHUNK_SIZE_Y;

    // 3. Bounds check
    if (chunk_x < 0 || chunk_x >= CHUNK_COUNT_X ||
        chunk_y < 0 || chunk_y >= CHUNK_COUNT_Y)
    {
        throw std::out_of_range("Global coordinates out of world bounds");
    }

    // 4. Convert chunk coordinates to index
    int chunk_index = chunk_y * CHUNK_SIZE_X + chunk_x;

    // 5. Return the element inside the chunk
    return m_chunks[chunk_index].at(local_x, local_y);
}


bool WorldManager::get_chunk_for_global_cell(
    int gx,
    int gy,
    int &out_x,
    int &out_y,
    int &out_lx,
    int &out_ly) const
{
    if (gx < 0 || gy < 0)
        return false;

    int cells_per_chunk_x = Chunk::m_width;
    int cells_per_chunk_y = Chunk::m_height;

    out_x = gx / cells_per_chunk_x;
    out_y = gy / cells_per_chunk_y;

    if (out_x < 0 || out_x >= CHUNK_COUNT_X ||
        out_y < 0 || out_y >= CHUNK_COUNT_Y)
        return false;

    out_lx = gx % cells_per_chunk_x;
    out_ly = gy % cells_per_chunk_y;
    return true;
}

void WorldManager::step() {

	std::vector<MoveDecision> cross;
	cross.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * 4); // tune

	// 1) per chunk
	for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy) {
	    for (int cx = 0; cx < CHUNK_COUNT_X; ++cx) {
	        Chunk& c = chunk_at(cx, cy);

	        const auto& decisions = c.process_movement();
	        auto outside = c.apply_decisions(decisions);  // inside swaps done here

	        cross.insert(cross.end(), outside.begin(), outside.end());
	    }
	}

	// 2) apply cross-chunk decisions
	if (cross.size() != 0)
	{
		apply_cross_chunk_swaps(cross);
	}
}



void WorldManager::apply_cross_chunk_swaps(const std::vector<MoveDecision>& cross)
{
    constexpr int W = Chunk::m_width;
    constexpr int H = Chunk::m_height;
    const int cells_per_chunk = W * H;

    // Per-chunk used masks (byte-per-cell, fast)
    static std::vector<uint8_t> used;
    used.assign(CHUNK_COUNT_X * CHUNK_COUNT_Y * cells_per_chunk, 0);

    auto used_for = [&](int cx, int cy) -> uint8_t* {
        return used.data() + (cy * CHUNK_COUNT_X + cx) * cells_per_chunk;
    };

    for (const MoveDecision& d : cross)
    {
        if (d.move_type != MoveType::Swap) continue;

        // --- 1) Convert local coords (possibly negative/oob) to GLOBAL ---
        const int base_gx = d.origin_cx * W;
        const int base_gy = d.origin_cy * H;

        const int old_gx = base_gx + d.old_x;
        const int old_gy = base_gy + d.old_y;
        const int new_gx = base_gx + d.new_x;
        const int new_gy = base_gy + d.new_y;

        // --- 2) Map GLOBAL -> owning chunk + local coords (works for negatives) ---
        const int old_cx = floor_div(old_gx, W);
        const int old_cy = floor_div(old_gy, H);
        const int new_cx = floor_div(new_gx, W);
        const int new_cy = floor_div(new_gy, H);

        // If you only simulate a fixed grid of chunks, skip moves that leave the world:
        if ((unsigned)old_cx >= (unsigned)CHUNK_COUNT_X || (unsigned)old_cy >= (unsigned)CHUNK_COUNT_Y ||
            (unsigned)new_cx >= (unsigned)CHUNK_COUNT_X || (unsigned)new_cy >= (unsigned)CHUNK_COUNT_Y)
        {
            continue;
        }

        const int old_lx = floor_mod(old_gx, W);
        const int old_ly = floor_mod(old_gy, H);
        const int new_lx = floor_mod(new_gx, W);
        const int new_ly = floor_mod(new_gy, H);

        // --- 3) Conflict protection (each cell max once) ---
        uint8_t* used_old = used_for(old_cx, old_cy);
        uint8_t* used_new = used_for(new_cx, new_cy);

        const int old_i = Chunk::index(old_lx, old_ly);
        const int new_i = Chunk::index(new_lx, new_ly);

        if (used_old[old_i] || used_new[new_i]) {
            // conflict policy: skip (fastest). alternative: retry list.
            continue;
        }

        used_old[old_i] = 1;
        used_new[new_i] = 1;

        // --- 4) Apply swap across chunks ---
        Chunk& c_old = chunk_at(old_cx, old_cy);
        Chunk& c_new = chunk_at(new_cx, new_cy);

        Element tmp = c_old.at(old_lx, old_ly);
        c_old.set_element_local(old_lx, old_ly, c_new.at(new_lx, new_ly));
        c_new.set_element_local(new_lx, new_ly, tmp);

        c_old.mark_dirty_external(old_lx, old_ly);
        c_new.mark_dirty_external(new_lx, new_ly);
    }
}

