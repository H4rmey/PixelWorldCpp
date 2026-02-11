#pragma once

#include "move_type.hpp"

struct MoveDecision {
	int old_x;
	int old_y;
	int new_x;
	int new_y;
	MoveType move_type = MoveType::Stay;
};
