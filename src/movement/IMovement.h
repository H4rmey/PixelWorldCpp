#pragma once

#include "../helpers/move_decision.h"
#include "../helpers/move_type.h"

// Forward declaration - Chunk will be fully defined when this is used
class Chunk;

class IMovement {
  public:
    virtual ~IMovement() = default;

    virtual MoveDecision process(int x, int y, const Chunk &chunk) const = 0;
};

// ---------------------------------------------------------
// EmptyMovement — does not move
// ---------------------------------------------------------
class EmptyMovement : public IMovement {
  public:
    MoveDecision process(int x, int y, const Chunk &chunk) const override {
        return {x, y, x, y, 0, 0, MoveType::Stay};
    }
};
