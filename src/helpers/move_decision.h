#pragma once

#include "move_type.h"

// struct MoveDecision {
//	int old_x;
//	int old_y;
//	int new_x;
//	int new_y;
//	MoveType move_type = MoveType::Stay;
// };

struct MoveDecision {
    int old_x, old_y;
    int new_x, new_y;
    int origin_cx, origin_cy; // chunk coords where this decision was created
    MoveType move_type = MoveType::Stay;

    static MoveDecision Stay(int x, int y) {
        return {x, y, x, y, 0, 0, MoveType::Stay};
    }
};
