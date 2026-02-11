#pragma once

#include "IMovement.hpp"
#include "chunk.hpp"

namespace godot {

// ---------------------------------------------------------
// SandMovement — does not move
// ---------------------------------------------------------
class SandMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_global(x, y);

		if (self.type != ElementType::Sand)
			return { x, y, x, y, MoveType::Stay };

		// Try to fall straight down
		const Element &below = chunk.get_element_global(x, y + 1);
		if (below.is_empty()) {
			return { x, y, x, y + 1, MoveType::Swap };
		}

		// Try down-left
		const Element &down_left = chunk.get_element_global(x - 1, y + 1);
		if (down_left.is_empty()) {
			return { x, y, x - 1, y + 1, MoveType::Swap };
		}

		// Try down-right
		const Element &down_right = chunk.get_element_global(x + 1, y + 1);
		if (down_right.is_empty()) {
			return { x, y, x + 1, y + 1, MoveType::Swap };
		}

		return { x, y, x, y, MoveType::Stay };
	}
};

// ---------------------------------------------------------
// SolidMovement — does not move
// ---------------------------------------------------------
class SolidMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		return { x, y, x, y, MoveType::Stay };
	}
};

// ---------------------------------------------------------
// GasMovement — rises upward, spreads sideways
// ---------------------------------------------------------
class GasMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_global(x, y);
		if (self.is_empty())
			return { x, y, x, y, MoveType::Stay };

		// Try up
		const Element &up = chunk.get_element_global(x, y - 1);
		if (up.is_empty())
			return { x, y, x, y - 1, MoveType::Move };

		// Try up-left
		const Element &up_left = chunk.get_element_global(x - 1, y - 1);
		if (up_left.is_empty())
			return { x, y, x - 1, y - 1, MoveType::Move };

		// Try up-right
		const Element &up_right = chunk.get_element_global(x + 1, y - 1);
		if (up_right.is_empty())
			return { x, y, x + 1, y - 1, MoveType::Move };

		// Try sideways
		const Element &left = chunk.get_element_global(x - 1, y);
		if (left.is_empty())
			return { x, y, x - 1, y, MoveType::Move };

		const Element &right = chunk.get_element_global(x + 1, y);
		if (right.is_empty())
			return { x, y, x + 1, y, MoveType::Move };

		return { x, y, x, y, MoveType::Stay };
	}
};

// ---------------------------------------------------------
// LiquidMovement — shared logic for water-like materials
// ---------------------------------------------------------
class LiquidMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_global(x, y);
		if (self.is_empty())
			return { x, y, x, y, MoveType::Stay };

		// Down
		const Element &down = chunk.get_element_global(x, y + 1);
		if (down.is_empty())
			return { x, y, x, y + 1, MoveType::Move };

		// Down-left
		const Element &down_left = chunk.get_element_global(x - 1, y + 1);
		if (down_left.is_empty())
			return { x, y, x - 1, y + 1, MoveType::Move };

		// Down-right
		const Element &down_right = chunk.get_element_global(x + 1, y + 1);
		if (down_right.is_empty())
			return { x, y, x + 1, y + 1, MoveType::Move };

		// Spread left
		const Element &left = chunk.get_element_global(x - 1, y);
		if (left.is_empty())
			return { x, y, x - 1, y, MoveType::Move };

		// Spread right
		const Element &right = chunk.get_element_global(x + 1, y);
		if (right.is_empty())
			return { x, y, x + 1, y, MoveType::Move };

		return { x, y, x, y, MoveType::Stay };
	}
};

// ---------------------------------------------------------
// WaterMovement — inherits LiquidMovement (optional)
// ---------------------------------------------------------
class WaterMovement : public LiquidMovement {
	// Inherits all behavior from LiquidMovement
};

// ---------------------------------------------------------
// PowderMovement — sand-like but lighter
// ---------------------------------------------------------
class PowderMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		return { x, y, x, y, MoveType::Stay };
	}
};

} // namespace godot
