#pragma once

//#include "chunk.hpp"
#include "element.hpp"
#include "constants.hpp"
#include "chunk.hpp"
#include "move_decision.hpp"
#include "move_type.hpp"

namespace godot {


class WorldManager {
private:
    static constexpr std::size_t m_size = CHUNK_COUNT;

	std::array<Chunk, CHUNK_COUNT> m_chunks;

public:
	WorldManager() {
		m_chunks.fill(Chunk());

		for (int cy = 0; cy < CHUNK_COUNT_X; ++cy) {
			for (int cx = 0; cx < CHUNK_COUNT_Y; ++cx) {
				Chunk &chunk = chunk_at(cx, cy);

				Chunk *left       = (cx > 0)											 ? &chunk_at(cx - 1, cy)	 : nullptr;
				Chunk *right      = (cx + 1 < CHUNK_COUNT_X)							 ? &chunk_at(cx + 1, cy)	 : nullptr;
				Chunk *up	      = (cy > 0)											 ? &chunk_at(cx, cy - 1)	 : nullptr;
				Chunk *down       = (cy + 1 < CHUNK_COUNT_Y)							 ? &chunk_at(cx, cy + 1)	 : nullptr;
				Chunk *up_left    = (cx > 0 && cy > 0)									 ? &chunk_at(cx - 1, cy - 1) : nullptr;
				Chunk *up_right   = (cx + 1 < CHUNK_COUNT_X && cy > 0)                   ? &chunk_at(cx + 1, cy - 1) : nullptr;
				Chunk *down_left  = (cx > 0 && cy + 1 < CHUNK_COUNT_Y)                   ? &chunk_at(cx - 1, cy + 1) : nullptr;
				Chunk *down_right = (cx + 1 < CHUNK_COUNT_X && cy + 1 < CHUNK_COUNT_Y)   ? &chunk_at(cx + 1, cy + 1) : nullptr;

				chunk.set_neighbor(NeighborDir::Left, left);
				chunk.set_neighbor(NeighborDir::Right, right);
				chunk.set_neighbor(NeighborDir::Up, up);
				chunk.set_neighbor(NeighborDir::Down, down);
				chunk.set_neighbor(NeighborDir::UpLeft, up_left);
				chunk.set_neighbor(NeighborDir::UpRight, up_right);
				chunk.set_neighbor(NeighborDir::DownLeft, down_left);
				chunk.set_neighbor(NeighborDir::DownRight, down_right);
			}
		}
	}

	Chunk &chunk_at(int cx, int cy) {
		return m_chunks[cy * CHUNK_COUNT_X + cx];
	}

	const Chunk &chunk_at(int cx, int cy) const {
		return m_chunks[cy * CHUNK_COUNT_X + cx];
	}

	// Map global cell coordinates to chunk + local coords
	bool get_chunk_for_global_cell(
			int gx,
			int gy,
			int &out_cx,
			int &out_cy,
			int &out_lx,
			int &out_ly) const {
		if (gx < 0 || gy < 0)
			return false;

		int cells_per_chunk_x = Chunk::m_width;
		int cells_per_chunk_y = Chunk::m_height;

		out_cx = gx / cells_per_chunk_x;
		out_cy = gy / cells_per_chunk_y;

		if (out_cx < 0 || out_cx >= CHUNK_COUNT_X ||
			out_cy < 0 || out_cy >= CHUNK_COUNT_Y)
			return false;

		out_lx = gx % cells_per_chunk_x;
		out_ly = gy % cells_per_chunk_y;
		return true;
	}

	// One full simulation step
	void step() {
		//std::array<std::vector<MoveDecision> *, CHUNK_COUNT_X * CHUNK_COUNT_Y> all_decisions{};

		// 1. Process movement in all chunks
		for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy) {
			for (int cx = 0; cx < CHUNK_COUNT_X; ++cx) {
				Chunk &chunk = chunk_at(cx, cy);
				auto &decisions = chunk.process_movement();
				chunk.apply_decisions(decisions);
			}
		}

		// 3. Route outgoing moves to destination chunks
		for (int cy = 0; cy < CHUNK_COUNT_Y; ++cy) {
			for (int cx = 0; cx < CHUNK_COUNT_X; ++cx) {
				Chunk &src_chunk = chunk_at(cx, cy);
				const auto &outgoing = src_chunk.get_outgoing_moves();

				for (const MoveDecision &d : outgoing) {
					int global_x = cx * Chunk::m_width + d.new_x;
					int global_y = cy * Chunk::m_height + d.new_y;

					int dcx, dcy, lx, ly;
					if (!get_chunk_for_global_cell(global_x, global_y, dcx, dcy, lx, ly))
						continue; // out of world

					Chunk &dst_chunk = chunk_at(dcx, dcy);

					switch (d.move_type) {
						case MoveType::Move: {
							//TODO: Implement
						} break;

						case MoveType::Swap: {
							// Compute global coordinates of both swap positions
							int global_old_x = cx * Chunk::m_width + d.old_x;
							int global_old_y = cy * Chunk::m_height + d.old_y;

							int global_new_x = cx * Chunk::m_width + d.new_x;
							int global_new_y = cy * Chunk::m_height + d.new_y;

							int ocx, ocy, olx, oly;
							int ncx, ncy, nlx, nly;

							// Map both positions to chunk/local coords
							if (!get_chunk_for_global_cell(global_old_x, global_old_y, ocx, ocy, olx, oly))
								break;
							if (!get_chunk_for_global_cell(global_new_x, global_new_y, ncx, ncy, nlx, nly))
								break;

							Chunk &old_chunk = chunk_at(ocx, ocy);
							Chunk &new_chunk = chunk_at(ncx, ncy);

							// Read both elements
							Element a = old_chunk.at(olx, oly);
							Element b = new_chunk.at(nlx, nly);

							// Swap them
							old_chunk.set_element_local(olx, oly, b);
							new_chunk.set_element_local(nlx, nly, a);

							// Mark dirty rects
							old_chunk.mark_dirty_external(olx, oly);
							new_chunk.mark_dirty_external(nlx, nly);

						} break;

						case MoveType::Stay:
							break;
					}
				}

				src_chunk.clear_outgoing_moves();
			}
		}
	}
};
} // godot namespace
