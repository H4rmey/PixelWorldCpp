#pragma once

#include "move_type.hpp"
#include "move_decision.hpp"

namespace godot {

// Forward declaration - Chunk will be fully defined when this is used
class Chunk;

class IMovement {
public:
	virtual ~IMovement() = default;

	virtual MoveDecision process(
			int x,
			int y,
			const Chunk &chunk) const = 0;

};
} // namespace godot
