#pragma once

#include "../helpers/move_decision.h"
#include "../helpers/move_type.h"
#include <cstdint>

// Forward declaration - Chunk will be fully defined when this is used
class Chunk;

class IMovement
{
  public:
    virtual ~IMovement() = default;

    virtual MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const = 0;
};

// ---------------------------------------------------------
// EmptyMovement — does not move
// ---------------------------------------------------------
class EmptyMovement : public IMovement
{
  public:
    MoveDecision process(uint8_t tick, int x, int y, Chunk &chunk) const override
    {
        return MoveDecision::Stay(x, y);
    }
};
