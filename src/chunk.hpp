#pragma once

#include <algorithm>
#include <array>
#include <vector>
#include <cstring>

#include "move_decision.hpp"
#include "IMovement.hpp"
#include "element_type.hpp"
#include "element.hpp"


namespace godot {

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

	bool is_valid() const {
		return max_x >= min_x && max_y >= min_y;
	}
};

class Chunk {
public:
	static constexpr int m_width = CHUNK_SIZE_X;
	static constexpr int m_height = CHUNK_SIZE_Y;

private:
	std::array<Element, CHUNK_SIZE_X * CHUNK_SIZE_Y> m_elements{};
	std::array<Element, CHUNK_SIZE_X * CHUNK_SIZE_Y> m_next_elements{};

	mutable std::vector<MoveDecision> m_decisions;
	mutable std::vector<MoveDecision> m_outgoing_moves;

	std::array<Chunk *, static_cast<size_t>(NeighborDir::COUNT)> m_neighbors{};

	Rect m_dirty_rect{};

public:
	Chunk() {
		m_decisions.reserve(m_width * m_height);
		m_outgoing_moves.reserve(m_width * m_height / 4);
		m_neighbors.fill(nullptr);
		m_elements.fill(Element());
	}

	~Chunk() = default;

	// ---------------------------------------------------------
	// Neighbor setup
	// ---------------------------------------------------------
	inline void set_neighbor(NeighborDir dir, Chunk *chunk) {
		m_neighbors[static_cast<size_t>(dir)] = chunk;
	}

	inline Chunk *get_neighbor(NeighborDir dir) const {
		return m_neighbors[static_cast<size_t>(dir)];
	}

	inline const Rect &get_dirty_rect() const { return m_dirty_rect; }

	// ---------------------------------------------------------
	// Index helpers
	// ---------------------------------------------------------
	inline int index(int x, int y) const {
		return y * m_width + x;
	}

	inline Element &at(int x, int y) {
		return m_elements[index(x, y)];
	}

	inline const Element &at(int x, int y) const {
		return m_elements[index(x, y)];
	}

	inline void set_element_local(int x, int y, const Element &e) {
		m_elements[index(x, y)] = e;
	}

	inline void mark_dirty_external(int x, int y) {
		if (!m_dirty_rect.is_valid()) {
			m_dirty_rect = { x, y, x, y };
		} else {
			m_dirty_rect.min_x = std::min(m_dirty_rect.min_x, x);
			m_dirty_rect.min_y = std::min(m_dirty_rect.min_y, y);
			m_dirty_rect.max_x = std::max(m_dirty_rect.max_x, x);
			m_dirty_rect.max_y = std::max(m_dirty_rect.max_y, y);
		}
	}

	inline const std::vector<MoveDecision> &get_outgoing_moves() const {
		return m_outgoing_moves;
	}

	inline void clear_outgoing_moves() const {
		m_outgoing_moves.clear();
	}

	void fill_with_empty(const IMovement *empty_movement) {
		for (Element &e : m_elements) {
			e = Element(ElementType::Empty, empty_movement);
		}
	}

	// ---------------------------------------------------------
	// Global element access (cross-chunk, read-only)
	// ---------------------------------------------------------
	const Element &get_element_global(int x, int y) const {
		// Fast path: inside this chunk
		if (x >= 0 && x < m_width &&
				y >= 0 && y < m_height) {
			return at(x, y);
		}

		// Determine which neighbor we need
		NeighborDir dir;
		int nx = x;
		int ny = y;

		if (x < 0 && y < 0) {
			dir = NeighborDir::UpLeft;
			nx += m_width;
			ny += m_height;
		} else if (x >= m_width && y < 0) {
			dir = NeighborDir::UpRight;
			nx -= m_width;
			ny += m_height;
		} else if (x < 0 && y >= m_height) {
			dir = NeighborDir::DownLeft;
			nx += m_width;
			ny -= m_height;
		} else if (x >= m_width && y >= m_height) {
			dir = NeighborDir::DownRight;
			nx -= m_width;
			ny -= m_height;
		} else if (x < 0) {
			dir = NeighborDir::Left;
			nx += m_width;
		} else if (x >= m_width) {
			dir = NeighborDir::Right;
			nx -= m_width;
		} else if (y < 0) {
			dir = NeighborDir::Up;
			ny += m_height;
		} else { // y >= m_height
			dir = NeighborDir::Down;
			ny -= m_height;
		}

		Chunk *target = m_neighbors[static_cast<size_t>(dir)];
		if (!target)
			return Element::Empty();

		return target->m_elements[target->index(nx, ny)];
	}

	// ---------------------------------------------------------
	// PROCESS PHASE
	// ---------------------------------------------------------
	std::vector<MoveDecision> &process_movement() const {
		m_decisions.clear();

		for (int y = 0; y < m_height; ++y) {
			for (int x = 0; x < m_width; ++x) {
				MoveDecision d = m_elements[this->index(x,y)].movement->process(x, y, *this);

				if (d.move_type != MoveType::Stay)
					m_decisions.push_back(d);
			}
		}

		return m_decisions;
	}

	// ---------------------------------------------------------
	// APPLY PHASE
	// ---------------------------------------------------------
	void apply_decisions(const std::vector<MoveDecision> &decisions) {
		// Track which rows need copying
		std::array<bool, m_height> dirty_rows{};
		dirty_rows.fill(false);

		// First pass: mark rows touched by decisions
		for (const MoveDecision &d : m_decisions) {
			if (d.old_y >= 0 && d.old_y < m_height)
				dirty_rows[d.old_y] = true;
			if (d.new_y >= 0 && d.new_y < m_height)
				dirty_rows[d.new_y] = true;
		}

		// Copy only dirty rows into next buffer
		for (int y = 0; y < m_height; ++y) {
			if (dirty_rows[y]) {
				const int row_start = y * m_width;
				std::memcpy(
						&m_next_elements[row_start],
						&m_elements[row_start],
						sizeof(Element) * m_width);
			}
		}

		// Dirty rect init
		int min_x = m_width, min_y = m_height;
		int max_x = -1, max_y = -1;

		auto mark_dirty = [&](int x, int y) {
			min_x = std::min(min_x, x);
			min_y = std::min(min_y, y);
			max_x = std::max(max_x, x);
			max_y = std::max(max_y, y);
		};

		auto in_local = [&](int x, int y) {
			return x >= 0 && x < m_width && y >= 0 && y < m_height;
		};

		m_outgoing_moves.clear();

		// Apply decisions
		for (const MoveDecision &d : decisions) {
			const int ox = d.old_x;
			const int oy = d.old_y;
			const int nx = d.new_x;
			const int ny = d.new_y;

			const int old_idx = oy * m_width + ox;

			switch (d.move_type) {
				case MoveType::Stay:
					break;

				case MoveType::Move:
					if (in_local(ox, oy)) {
						if (in_local(nx, ny)) {
							const int new_idx = ny * m_width + nx;
							m_next_elements[new_idx] = m_elements[old_idx];
							m_next_elements[old_idx] = Element::Empty();
							mark_dirty(ox, oy);
							mark_dirty(nx, ny);
						} else {
							m_outgoing_moves.push_back(d);
							m_next_elements[old_idx] = Element::Empty();
							mark_dirty(ox, oy);
						}
					}
					break;

				case MoveType::Swap:
					if (in_local(ox, oy) && in_local(nx, ny)) {
						const int new_idx = ny * m_width + nx;
						std::swap(m_next_elements[old_idx], m_next_elements[new_idx]);
						mark_dirty(ox, oy);
						mark_dirty(nx, ny);
					}
					break;
			}
		}

		// Swap buffers
		m_elements.swap(m_next_elements);

		// Update dirty rect
		if (max_x >= 0)
			m_dirty_rect = { min_x, min_y, max_x, max_y };
		else
			m_dirty_rect = {};
	}
};
} // godot namespace
