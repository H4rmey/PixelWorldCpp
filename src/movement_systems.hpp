#pragma once

#include "chunk.hpp"
#include "helpers/element_type.h"
#include "helpers/move_decision.h"
#include "movement/IMovement.h"

#include <random>

inline bool fastRandomBool(float distribution) {
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(distribution);
    return dist(rng);
}

inline bool fastRandomBool() {
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(0.5);
    return dist(rng);
}

inline int fastRandomIntRange(int min = 0, int max = 255) {
    static thread_local std::mt19937 rng(std::random_device{}());
    // Note: distribution range must be set at construction or passed as param
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloatRange(float min = 0.0f, float max = 1.0f) {
    static thread_local std::mt19937      rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloat01() {
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng);
}

inline int density(ElementType t) {
    switch (t) {
    case ElementType::Gas:
        return 1;
    case ElementType::Water:
        return 5;
    case ElementType::Sand:
        return 10;
    case ElementType::Stone:
        return 1000;
    default:
        return 0;
    }
}
inline bool can_swap(const Element &a, const Element &b) {
    // allow swapping into empty always
    if (b.is_empty())
        return true;
    // heavier falls through lighter
    return density(a.type) > density(b.type);
}

// ---------------------------------------------------------
// SandMovement — does not move
// ---------------------------------------------------------

class SandMovement : public IMovement {
  public:
    MoveDecision process(int x, int y, Chunk &chunk) const override {
        // get the element at the location
        Element &self = chunk.get_element_local(x, y);

        if (!self.physics.canMove_y) {
            // kill momentum
            return MoveDecision::Stay(x, y);
        }

        auto canSwap = [&](int nx, int ny) -> bool {
            Element &t = chunk.get_element_local(nx, ny);
            if (t.is_empty()) {
                return true;
            }
            return density(self.type) > density(t.type);
        };

        auto allowMoveX = [&](int nx, int ny) {
            int global_x;
            int global_y;
            chunk.get_global_coords(nx, ny, global_x, global_y);

            int dx[] = {0, 0, -1, 1};
            int dy[] = {-1, 1, 0, 0};

            for (int i = 0; i < 4; ++i) {
                int neighbor_gx = global_x + dx[i];
                int neighbor_gy = global_y + dy[i];

                Element &neighbor =
                    chunk.get_element_global(neighbor_gx, neighbor_gy);

                if (fastRandomBool(neighbor.physics.stability)) {
                    neighbor.physics.canMove_x = true;
                }
            }
        };

        // Down
        if (canSwap(x, y + 1)) {
            self.physics.terminal_velocity_y++;
            allowMoveX(x, y);
            return {x, y, x, y + 1, 0, 0, MoveType::Swap};
        }

        // cancel the horizontal movement
        if (!self.physics.canMove_x) {
            // kill momentum
            return MoveDecision::Stay(x, y);
        }

        if (fastRandomBool(self.physics.friction)) {
            // kill momentum
            self.physics.canMove_x = false;
            return MoveDecision::Stay(x, y);
        }

        // Randomize diagonals
        bool   leftFirst = fastRandomBool();
        int8_t dx1       = leftFirst ? -1 : 1;

        // move down to the left
        if (canSwap(x + dx1, y + 1)) {
            self.physics.terminal_velocity_y++;
            return {x, y, x + dx1, y + 1, 0, 0, MoveType::Swap};
        }
        // move down to the right
        if (canSwap(x - dx1, y + 1)) {
            self.physics.terminal_velocity_y++;
            return {x, y, x - dx1, y + 1, 0, 0, MoveType::Swap};
        }

        // if the resolve direction is 0
        // 	this sets a resolve direction so the pixel does not move
        //	side to side after landing
        if (self.physics.velocity_resolve_dir_x == 0) {
            self.physics.velocity_resolve_dir_x = fastRandomBool() ? -1 : 1;
        }
        if (self.physics.terminal_velocity_y > 0) {
            self.physics.terminal_velocity_y--;
            int dx1 = self.physics.velocity_resolve_dir_x;
            if (canSwap(x + dx1, y)) {
                return {x, y, x + dx1, y, 0, 0, MoveType::Swap};
            }
        }

        self.physics.terminal_velocity_y    = 0;
        self.physics.velocity_resolve_dir_x = 0;
        return MoveDecision::Stay(x, y);
    }
};

// ---------------------------------------------------------
// SolidMovement — does not move
// ---------------------------------------------------------
class SolidMovement : public IMovement {
  public:
    MoveDecision process(int x, int y, Chunk &chunk) const override {
        return MoveDecision::Stay(x, y);
    }
};

// ---------------------------------------------------------
// GasMovement — rises upward, spreads sideways
// ---------------------------------------------------------
class GasMovement : public IMovement {
  public:
    MoveDecision process(int x, int y, Chunk &chunk) const override {
        const Element &self = chunk.get_element_local(x, y);
        if (self.is_empty())
            return MoveDecision::Stay(x, y);

        // Try up
        const Element &up = chunk.get_element_local(x, y - 1);
        if (up.is_empty())
            return {x, y, x, y - 1, 0, 0, MoveType::Swap};

        // Try up-left
        const Element &up_left = chunk.get_element_local(x - 1, y - 1);
        if (up_left.is_empty())
            return {x, y, x - 1, y - 1, 0, 0, MoveType::Swap};

        // Try up-right
        const Element &up_right = chunk.get_element_local(x + 1, y - 1);
        if (up_right.is_empty())
            return {x, y, x + 1, y - 1, 0, 0, MoveType::Swap};

        // Try sideways
        const Element &left = chunk.get_element_local(x - 1, y);
        if (left.is_empty())
            return {x, y, x - 1, y, 0, 0, MoveType::Swap};

        const Element &right = chunk.get_element_local(x + 1, y);
        if (right.is_empty())
            return {x, y, x + 1, y, 0, 0, MoveType::Swap};

        return MoveDecision::Stay(x, y);
    }
};

// ---------------------------------------------------------
// LiquidMovement — shared logic for water-like materials
// ---------------------------------------------------------
class LiquidMovement : public IMovement {
  public:
    MoveDecision process(int x, int y, Chunk &chunk) const override {
        const Element &self = chunk.get_element_local(x, y);
        if (self.is_empty())
            return MoveDecision::Stay(x, y);

        auto canSwap = [&](int nx, int ny) -> bool {
            const Element &t = chunk.get_element_local(nx, ny);
            if (t.is_empty())
                return true;
            return density(self.type) > density(t.type);
        };

        // Down / diagonals first
        if (canSwap(x, y + 1))
            return {x, y, x, y + 1, 0, 0, MoveType::Swap};

        bool leftFirst = fastRandomBool();
        int  dx1       = leftFirst ? -1 : 1;
        int  dx2       = -dx1;

        if (canSwap(x + dx1, y + 1))
            return {x, y, x + dx1, y + 1, 0, 0, MoveType::Swap};
        if (canSwap(x + dx2, y + 1))
            return {x, y, x + dx2, y + 1, 0, 0, MoveType::Swap};

        // Lateral spread with simple "pressure"
        static constexpr int MAX_SPREAD = 5;

        auto score = [&](int nx) -> int {
            // Prefer cells where it can fall next tick
            const Element &below = chunk.get_element_local(nx, y + 1);
            return (below.is_empty() ||
                    density(self.type) > density(below.type))
                       ? 1
                       : 0;
        };

        // Try best direction first, but still unbiased
        int bestDir = (score(x + dx1) >= score(x + dx2)) ? dx1 : dx2;
        for (int pass = 0; pass < 2; ++pass) {
            int dir = (pass == 0) ? bestDir : -bestDir;
            for (int i = 1; i <= MAX_SPREAD; ++i) {
                int            nx = x + dir * i;
                const Element &t  = chunk.get_element_local(nx, y);
                if (canSwap(nx, y))
                    return {x, y, nx, y, 0, 0, MoveType::Swap};

                // stop when hitting an immovable wall
                if (!t.is_empty() &&
                    density(t.type) >= density(ElementType::Stone))
                    break;
            }
        }

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
    MoveDecision process(int x, int y, Chunk &chunk) const override {
        return MoveDecision::Stay(x, y);
    }
};
