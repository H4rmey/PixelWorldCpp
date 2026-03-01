#pragma once

#include "helpers/move_decision.h"
#include "movement/IMovement.h"
#include "chunk.hpp"
#include "helpers/element_type.h"

// ---------------------------------------------------------
// SandMovement — does not move
// ---------------------------------------------------------
class SandMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_local(x, y);

		if (self.type != ElementType::Sand)
			return MoveDecision::Stay(x,y);

		// Try to fall straight down
		const Element &below = chunk.get_element_local(x, y + 1);
		if (below.is_empty()) {
			return { x, y, x, y + 1, 0, 0, MoveType::Swap };
		}

		// Try down-left
		const Element &down_left = chunk.get_element_local(x - 1, y + 1);
		if (down_left.is_empty()) {
			return { x, y, x - 1, y + 1, 0, 0, MoveType::Swap };
		}

		// Try down-right
		const Element &down_right = chunk.get_element_local(x + 1, y + 1);
		if (down_right.is_empty()) {
			return { x, y, x + 1, y + 1, 0, 0, MoveType::Swap };
		}

		return MoveDecision::Stay(x, y);
	}
};

// ---------------------------------------------------------
// SolidMovement — does not move
// ---------------------------------------------------------
class SolidMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		return MoveDecision::Stay(x, y);
	}
};

// ---------------------------------------------------------
// GasMovement — rises upward, spreads sideways
// ---------------------------------------------------------
class GasMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_local(x, y);
		if (self.is_empty())
			return MoveDecision::Stay(x, y);

		// Try up
		const Element &up = chunk.get_element_local(x, y - 1);
		if (up.is_empty())
			return { x, y, x, y - 1, 0, 0, MoveType::Swap };

		// Try up-left
		const Element &up_left = chunk.get_element_local(x - 1, y - 1);
		if (up_left.is_empty())
			return { x, y, x - 1, y - 1, 0, 0, MoveType::Swap };

		// Try up-right
		const Element &up_right = chunk.get_element_local(x + 1, y - 1);
		if (up_right.is_empty())
			return { x, y, x + 1, y - 1, 0, 0, MoveType::Swap };

		// Try sideways
		const Element &left = chunk.get_element_local(x - 1, y);
		if (left.is_empty())
			return { x, y, x - 1, y, 0, 0, MoveType::Swap };

		const Element &right = chunk.get_element_local(x + 1, y);
		if (right.is_empty())
			return { x, y, x + 1, y, 0, 0, MoveType::Swap };

		return MoveDecision::Stay(x, y);
	}
};

// ---------------------------------------------------------
// LiquidMovement — shared logic for water-like materials
// ---------------------------------------------------------
class LiquidMovement : public IMovement {
public:
	MoveDecision process(int x, int y, const Chunk &chunk) const override {
		const Element &self = chunk.get_element_local(x, y);
		if (self.is_empty())
			return MoveDecision::Stay(x, y);

		// Down
		const Element &down = chunk.get_element_local(x, y + 1);
		if (down.is_empty())
			return { x, y, x, y + 1, 0, 0, MoveType::Swap };

		// Down-left
		const Element &down_left = chunk.get_element_local(x - 1, y + 1);
		if (down_left.is_empty())
			return { x, y, x - 1, y + 1, 0, 0, MoveType::Swap };

		// Down-right
		const Element &down_right = chunk.get_element_local(x + 1, y + 1);
		if (down_right.is_empty())
			return { x, y, x + 1, y + 1, 0, 0, MoveType::Swap };

		// Spread left
		const Element &left = chunk.get_element_local(x - 1, y);
		if (left.is_empty())
			return { x, y, x - 1, y, 0, 0, MoveType::Swap };

		// Spread right
		const Element &right = chunk.get_element_local(x + 1, y);
		if (right.is_empty())
			return { x, y, x + 1, y, 0, 0, MoveType::Swap };

		return MoveDecision::Stay(x, y);
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
		return MoveDecision::Stay(x, y);
	}
};
