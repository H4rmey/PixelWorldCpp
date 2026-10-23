#pragma once

#include "chunk.hpp"
#include "helpers/constants.h"
#include "helpers/element_type.h"
#include "helpers/matter_state.h"
#include "helpers/move_decision.h"
#include "movement/IMovement.h"

#include <random>

inline bool fastRandomBool(float distribution)
{
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(distribution);
    return dist(rng);
}

inline bool fastRandomBool()
{
    static thread_local std::mt19937                rng(std::random_device{}());
    static thread_local std::bernoulli_distribution dist(0.5);
    return dist(rng);
}

inline int fastRandomIntRange(int min = 0, int max = 255)
{
    static thread_local std::mt19937 rng(std::random_device{}());
    // Note: distribution range must be set at construction or passed as param
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloatRange(float min = 0.0f, float max = 1.0f)
{
    static thread_local std::mt19937      rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}
inline float fastRandomFloat01()
{
    static thread_local std::mt19937                          rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng);
}

// ---------------------------------------------------------
// Scaffolding
// ---------------------------------------------------------
class ScaffoldingMovement : public IMovement
{
  public:
    static constexpr int MAX_HORIZONTAL_CHAIN = 8;

    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        Element &self = chunk.get_element_local(x, y);

        if ((int)(tick * self.physics.mass) % GRAVITY >= self.physics.mass)
        {
            return {x, y, x, y, 0, 0, MoveType::Processing};
        }

        // 1. Chunk Activation (Mirroring the C# side-chunk logic)
        chunk.activate_cardinal_neighbors();

        // Reset flags for this frame's calculation
        self.physics.is_v_anchor = false;
        self.physics.is_h_anchor = false;

        int gx, gy;
        chunk.get_global_coords(x, y, gx, gy);

        // 2. Check directly below
        Element &below = chunk.get_element_global(gx, gy + 1);

        // If below is Solid or World Boundary, it's a Vertical Anchor
        if (below.state == MatterState::Solid || below.type == ElementType::OutOfBounds)
        {
            self.physics.is_v_anchor = true;
            return MoveDecision::Stay(x, y);
        }

        // 3. Check if landing on stable scaffolding
        if (below.type == ElementType::Scaffolding)
        {
            if (below.physics.is_v_anchor || below.physics.is_h_anchor)
            {
                self.physics.is_v_anchor = true;
                return MoveDecision::Stay(x, y);
            }
        }

        // 4. Horizontal Stability (Left and Right chains)
        if (check_horizontal_stability(gx, gy, -1, chunk) ||
            check_horizontal_stability(gx, gy, 1, chunk))
        {
            self.physics.is_h_anchor = true;
            return MoveDecision::Stay(x, y);
        }

        // 5. Falling logic
        if (self.physics.density > below.physics.density)
        {
            return {x, y, x, y + 1, 0, 0, MoveType::Swap};
        }

        // If it's not empty but also not stable (e.g., on top of falling sand),
        // it stays for now
        return MoveDecision::Stay(x, y);
    }

  private:
    bool check_horizontal_stability(int gx, int gy, int dir, Chunk &chunk) const
    {
        for (int i = 1; i <= MAX_HORIZONTAL_CHAIN; ++i)
        {
            int      cur_gx      = gx + (i * dir);
            Element &check_pixel = chunk.get_element_global(cur_gx, gy);

            if (check_pixel.type != ElementType::Scaffolding)
            {
                break; // Chain is broken by empty space or other elements
            }

            // Check if this scaffolding piece in the chain has vertical support
            if (check_pixel.physics.is_v_anchor)
            {
                Element &below_check = chunk.get_element_global(cur_gx, gy + 1);

                // If it has support from below, the whole horizontal chain is
                // stable
                if (below_check.type == ElementType::Scaffolding ||
                    below_check.type == ElementType::OutOfBounds ||
                    below_check.state == MatterState::Solid)
                {
                    return true;
                }
            }
        }
        return false;
    }
};
// ---------------------------------------------------------
// SandMovement — does not move
// ---------------------------------------------------------

class SandMovement : public IMovement
{
  public:
    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        // get the element at the location
        Element &self = chunk.get_element_local(x, y);

        // if ((int)(tick * self.physics.mass) % GRAVITY >= self.physics.mass)
        if ((int)(tick * self.physics.mass) % GRAVITY >= self.physics.mass)
        {
            return {x, y, x, y, 0, 0, MoveType::Processing};
        }

        if (!self.physics.canMove_y)
        {
            // kill momentum
            return MoveDecision::Stay(x, y);
        }

        auto canSwap = [&](int nx, int ny) -> bool
        {
            Element &t = chunk.get_element_local(nx, ny);
            // Sand (density 160) should always displace Water (density 100)
            return t.is_empty() || self.physics.density > t.physics.density;
        };

        auto allowMoveX = [&](int nx, int ny)
        {
            int global_x;
            int global_y;
            chunk.get_global_coords(nx, ny, global_x, global_y);

            int dx[] = {0, 0, -1, 1};
            int dy[] = {-1, 1, 0, 0};

            for (int i = 0; i < 4; ++i)
            {
                int neighbor_gx = global_x + dx[i];
                int neighbor_gy = global_y + dy[i];

                Element &neighbor = chunk.get_element_global(neighbor_gx, neighbor_gy);

                if (fastRandomBool(neighbor.physics.stability))
                {
                    neighbor.physics.canMove_x = true;
                }
            }
        };

        // Down
        if (canSwap(x, y + 1))
        {
            self.physics.terminal_velocity_y++;
            allowMoveX(x, y);
            return {x, y, x, y + 1, 0, 0, MoveType::Swap};
        }

        // cancel the horizontal movement
        if (!self.physics.canMove_x)
        {
            // kill momentum
            return MoveDecision::Stay(x, y);
        }

        if (fastRandomBool(self.physics.friction_x))
        {
            // kill momentum
            self.physics.canMove_x = false;
            return MoveDecision::Stay(x, y);
        }

        // Randomize diagonals
        bool   leftFirst = fastRandomBool();
        int8_t dx1       = leftFirst ? -1 : 1;

        // move down to the left
        if (canSwap(x + dx1, y + 1))
        {
            self.physics.terminal_velocity_y++;
            return {x, y, x + dx1, y + 1, 0, 0, MoveType::Swap};
        }
        // move down to the right
        if (canSwap(x - dx1, y + 1))
        {
            self.physics.terminal_velocity_y++;
            return {x, y, x - dx1, y + 1, 0, 0, MoveType::Swap};
        }

        // if the resolve direction is 0
        // 	this sets a resolve direction so the pixel does not move
        //	side to side after landing
        if (self.physics.velocity_resolve_dir_x == 0)
        {
            self.physics.velocity_resolve_dir_x = fastRandomBool() ? -1 : 1;
        }
        if (self.physics.terminal_velocity_y > 0)
        {
            self.physics.terminal_velocity_y--;
            int dx1 = self.physics.velocity_resolve_dir_x;
            if (canSwap(x + dx1, y))
            {
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
class SolidMovement : public IMovement
{
  public:
    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        return MoveDecision::Stay(x, y);
    }
};

// ---------------------------------------------------------
// GasMovement — rises upward, spreads sideways
// ---------------------------------------------------------
class GasMovement : public IMovement
{
  public:
    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        const Element &self = chunk.get_element_local(x, y);

        if ((int)(tick * self.physics.mass) % GRAVITY >= self.physics.mass)
        {
            return MoveDecision::Processing(x, y);
        }

        if (self.is_empty())
        {
            return MoveDecision::Stay(x, y);
        }

        // Try up
        const Element &up = chunk.get_element_local(x, y - 1);
        if (up.is_empty())
        {
            return {x, y, x, y - 1, 0, 0, MoveType::Swap};
        }

        // Try up-left
        const Element &up_left = chunk.get_element_local(x - 1, y - 1);
        if (up_left.is_empty())
        {
            return {x, y, x - 1, y - 1, 0, 0, MoveType::Swap};
        }

        // Try up-right
        const Element &up_right = chunk.get_element_local(x + 1, y - 1);
        if (up_right.is_empty())
        {
            return {x, y, x + 1, y - 1, 0, 0, MoveType::Swap};
        }

        // Try sideways
        const Element &left = chunk.get_element_local(x - 1, y);
        if (left.is_empty())
        {
            return {x, y, x - 1, y, 0, 0, MoveType::Swap};
        }

        const Element &right = chunk.get_element_local(x + 1, y);
        if (right.is_empty())
        {
            return {x, y, x + 1, y, 0, 0, MoveType::Swap};
        }

        return MoveDecision::Stay(x, y);
    }
};

// ---------------------------------------------------------
// LiquidMovement — shared logic for water-like materials
// ---------------------------------------------------------
class LiquidMovement : public IMovement
{
  public:
    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        Element &self = chunk.get_element_local(x, y);

        if ((int)(tick * self.physics.mass) % GRAVITY >= self.physics.mass)
        {
            return MoveDecision::Processing(x, y);
        }

        if (!self.physics.canMove_y)
        {
            return MoveDecision::Stay(x, y);
        }

        int gx, gy;
        chunk.get_global_coords(x, y, gx, gy);

        // --- 1. Downward Check ---
        Element &below = chunk.get_element_global(gx, gy + 1);
        if (below.is_empty() || self.physics.density > below.physics.density)
        {
            self.physics.terminal_velocity_y++;
            return {x, y, x, y + 1, 0, 0, MoveType::Swap};
        }

        // --- 2. Stability/Friction Check ---
        // If we have no vertical velocity and are on a flat surface,
        // add a chance to stay still. This prevents "infinite sloshing."
        if (self.physics.terminal_velocity_y == 0)
        {
            // Using a "friction" or "viscosity" check to settle
            // If fastRandomBool(70) returns true 70% of the time,
            // the liquid is more "syrupy" and settles faster.
            if (fastRandomBool(self.physics.friction_x))
            {
                return MoveDecision::Stay(x, y);
            }
        }

        // --- 3. Lateral Flow ---
        bool move_left = fastRandomBool();
        int  dir       = move_left ? -1 : 1;

        // Try to find a spot. We pass 'true' to prioritize holes over flat
        // ground.
        int next_gx = find_lateral_pos(gx, gy, dir, self.physics.viscosity, chunk);
        if (next_gx == gx)
        {
            next_gx = find_lateral_pos(gx, gy, -dir, self.physics.viscosity, chunk);
        }

        if (next_gx != gx)
        {
            int local_target_x = x + (next_gx - gx);
            // Kill vertical momentum when moving sideways
            self.physics.terminal_velocity_y = 0;
            return {x, y, local_target_x, y, 0, 0, MoveType::Swap};
        }

        // --- 4. Resolve ---
        self.physics.terminal_velocity_y = 0;
        return MoveDecision::Stay(x, y);
    }

  private:
    int find_lateral_pos(int gx, int gy, int dir, int viscosity, Chunk &chunk) const
    {
        int last_valid_gx = gx;

        for (int i = 1; i <= viscosity; ++i)
        {
            int      target_gx = gx + (i * dir);
            Element &target    = chunk.get_element_global(target_gx, gy);

            // Path is blocked
            if (!target.is_empty())
            {
                break;
            }

            // Path is empty!
            last_valid_gx = target_gx;
        }

        // If we are here, we found a flat path.
        // To prevent gaps, only move to the IMMEDIATE neighbor if no hole was
        // found. Or, use a probability to decide if we should jump the full distance.
        if (last_valid_gx != gx)
        {
            // Only move ONE pixel at a time if there's no hole to fall into.
            // This prevents "leapfrogging" and leaves no gaps.
            return gx + dir;
        }

        return gx;
    }
};

// ---------------------------------------------------------
// WaterMovement — inherits LiquidMovement (optional)
// ---------------------------------------------------------
class WaterMovement : public LiquidMovement
{
    // Inherits all behavior from LiquidMovement
};
