#pragma once

#include "helpers/move_decision.h"
#include "movement/IMovement.h"
#include "chunk.hpp"
#include "helpers/element_type.h"


#include <cstdint>
#include <limits>

// PCG-inspired 32-bit hash (the common variant includes the extra multiply)
static inline uint32_t pcg_hash(uint32_t input)
{
    uint32_t state = input * 747796405u + 2891336453u;
    uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Convert 32-bit uint to uniform float in [0,1).
// Uses the top 24 bits (exactly representable in float) to avoid precision bias.
static inline float u32_to_unit_float(uint32_t x)
{
    // Take top 24 bits -> integer in [0, 2^24-1]
    // Multiply by 1/2^24 to get [0,1)
    return (x >> 8) * (1.0f / 16777216.0f); // 2^24
}

// Good coin flip: hash the seed, then use 1 bit (or compare to half).
static inline bool coin_flip(uint32_t seed)
{
    return (pcg_hash(seed) & 1u) != 0u;
}

// If you want a float threshold coin flip:
static inline bool coin_flip_p(uint32_t seed, float p)
{
    // p in [0,1]
    return u32_to_unit_float(pcg_hash(seed)) < p;
}

static inline uint32_t seed_from_cell(const Chunk& chunk, int x, int y, uint32_t frame = 0u)
{
    // Convert to world-space cell coords (adjust these if your world_pos is already in cells)
    const uint32_t wx = (uint32_t)(chunk.get_world_pos_x() + x);
    const uint32_t wy = (uint32_t)(chunk.get_world_pos_y() + y);

    // Mix with distinct odd constants (spatial hash style), then run pcg_hash
    uint32_t s = wx * 73856093u ^ wy * 19349663u ^ frame * 83492791u;
    return pcg_hash(s);
}

static inline bool coin_flip_cell(const Chunk& chunk, int x, int y, uint32_t frame = 0u)
{
    return (seed_from_cell(chunk, x, y, frame) & 1u) != 0u;
}

inline int density(ElementType t) {
    switch (t) {
      case ElementType::Gas:   return 1;
      case ElementType::Water: return 5;
      case ElementType::Sand:  return 10;
	  case ElementType::Stone: return 1000;
      default: return 0;
    }
}
inline bool can_swap(const Element& a, const Element& b) {
    // allow swapping into empty always
    if (b.is_empty()) return true;
    // heavier falls through lighter
    return density(a.type) > density(b.type);
}
//inline bool coin_flip(int x, int y, uint32_t frame) {
//    uint32_t h = 2166136261u;
//    h = (h ^ (uint32_t)x) * 16777619u;
//    h = (h ^ (uint32_t)y) * 16777619u;
//    h = (h ^ frame) * 16777619u;
//    return (h & 1u) != 0u;
//}


// ---------------------------------------------------------
// SandMovement — does not move
// ---------------------------------------------------------

class SandMovement : public IMovement {
public:
    MoveDecision process(int x, int y, const Chunk& chunk) const override {
        const Element& self = chunk.get_element_local(x, y);
        if (self.type != ElementType::Sand) return MoveDecision::Stay(x, y);

        auto canSwap = [&](int nx, int ny) -> bool {
            const Element& t = chunk.get_element_local(nx, ny);
            if (t.is_empty()) return true;
            return density(self.type) > density(t.type);
        };

        // Down
        if (canSwap(x, y+1)) return {x,y,x,y+1,0,0,MoveType::Swap};

        // Randomize diagonals
		bool leftFirst = coin_flip_cell(chunk, x, y /*, frame */);
		std::printf("bool: %d\n", leftFirst);
        int dx1 = leftFirst ? -1 : 1;
        int dx2 = -dx1;

        if (canSwap(x+dx1, y+1)) return {x,y,x+dx1,y+1,0,0,MoveType::Swap};
        if (canSwap(x+dx2, y+1)) return {x,y,x+dx2,y+1,0,0,MoveType::Swap};

        // Optional: slide along ledges (lookahead)
        if (chunk.get_element_local(x-1, y).is_empty() && canSwap(x-1, y+1))
            return {x,y,x-1,y,0,0,MoveType::Swap};
        if (chunk.get_element_local(x+1, y).is_empty() && canSwap(x+1, y+1))
            return {x,y,x+1,y,0,0,MoveType::Swap};

        return MoveDecision::Stay(x, y);
    }
};
//class SandMovement : public IMovement {
//public:
//	MoveDecision process(int x, int y, const Chunk &chunk) const override {
//		const Element &self = chunk.get_element_local(x, y);
//
//		if (self.type != ElementType::Sand)
//			return MoveDecision::Stay(x,y);
//
//		// Try to fall straight down
//		const Element &below = chunk.get_element_local(x, y + 1);
//		if (below.is_empty()) {
//			return { x, y, x, y + 1, 0, 0, MoveType::Swap };
//		}
//
//		// Try down-left
//		const Element &down_left = chunk.get_element_local(x - 1, y + 1);
//		if (down_left.is_empty()) {
//			return { x, y, x - 1, y + 1, 0, 0, MoveType::Swap };
//		}
//
//		// Try down-right
//		const Element &down_right = chunk.get_element_local(x + 1, y + 1);
//		if (down_right.is_empty()) {
//			return { x, y, x + 1, y + 1, 0, 0, MoveType::Swap };
//		}
//
//		return MoveDecision::Stay(x, y);
//	}
//};

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
    MoveDecision process(int x, int y, const Chunk& chunk) const override {
        const Element& self = chunk.get_element_local(x, y);
        if (self.is_empty()) return MoveDecision::Stay(x, y);

        auto canSwap = [&](int nx, int ny) -> bool {
            const Element& t = chunk.get_element_local(nx, ny);
            if (t.is_empty()) return true;
            return density(self.type) > density(t.type);
        };

        // Down / diagonals first
        if (canSwap(x, y+1)) return {x,y,x,y+1,0,0,MoveType::Swap};

		bool leftFirst = coin_flip_cell(chunk, x, y /*, frame */);
        int dx1 = leftFirst ? -1 : 1;
        int dx2 = -dx1;

        if (canSwap(x+dx1, y+1)) return {x,y,x+dx1,y+1,0,0,MoveType::Swap};
        if (canSwap(x+dx2, y+1)) return {x,y,x+dx2,y+1,0,0,MoveType::Swap};

        // Lateral spread with simple "pressure"
        static constexpr int MAX_SPREAD = 5;

        auto score = [&](int nx) -> int {
            // Prefer cells where it can fall next tick
            const Element& below = chunk.get_element_local(nx, y+1);
            return (below.is_empty() || density(self.type) > density(below.type)) ? 1 : 0;
        };

        // Try best direction first, but still unbiased
        int bestDir = (score(x+dx1) >= score(x+dx2)) ? dx1 : dx2;
        for (int pass=0; pass<2; ++pass) {
            int dir = (pass==0) ? bestDir : -bestDir;
            for (int i=1; i<=MAX_SPREAD; ++i) {
                int nx = x + dir*i;
                const Element& t = chunk.get_element_local(nx, y);
                if (canSwap(nx, y)) return {x,y,nx,y,0,0,MoveType::Swap};

                // stop when hitting an immovable wall
                if (!t.is_empty() && density(t.type) >= density(ElementType::Stone))
                    break;
            }
        }

        return MoveDecision::Stay(x, y);
    }
};
//class LiquidMovement : public IMovement {
//public:
//	MoveDecision process(int x, int y, const Chunk &chunk) const override {
//		const Element &self = chunk.get_element_local(x, y);
//		if (self.is_empty())
//			return MoveDecision::Stay(x, y);
//
//		// Down
//		const Element &down = chunk.get_element_local(x, y + 1);
//		if (down.is_empty())
//			return { x, y, x, y + 1, 0, 0, MoveType::Swap };
//
//		// Down-left
//		const Element &down_left = chunk.get_element_local(x - 1, y + 1);
//		if (down_left.is_empty())
//			return { x, y, x - 1, y + 1, 0, 0, MoveType::Swap };
//
//		// Down-right
//		const Element &down_right = chunk.get_element_local(x + 1, y + 1);
//		if (down_right.is_empty())
//			return { x, y, x + 1, y + 1, 0, 0, MoveType::Swap };
//
//		// Spread left
//		const Element &left = chunk.get_element_local(x - 1, y);
//		if (left.is_empty())
//			return { x, y, x - 1, y, 0, 0, MoveType::Swap };
//
//		// Spread right
//		const Element &right = chunk.get_element_local(x + 1, y);
//		if (right.is_empty())
//			return { x, y, x + 1, y, 0, 0, MoveType::Swap };
//
//		return MoveDecision::Stay(x, y);
//	}
//};

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
